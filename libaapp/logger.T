// -*-c++-*-

#include "sfs_logger.h"

//-----------------------------------------------------------------------

sfs::logger_t::logger_t (str n, int m, int t) 
  : _file (n), _mode (m), _lock (tame::lock_t::OPEN), _tries (t), _pid (-1),
    _destroyed (New refcounted<bool> (false)) {}

//-----------------------------------------------------------------------

sfs::logger_t::~logger_t ()
{
  warn << "killing sfs_logger[" << _pid << "]\n";
  *_destroyed = true;
  _pid = -1;
  _x = NULL;
  _cli = NULL;
}

//-----------------------------------------------------------------------

void
sfs::logger_t::eofcb (ptr<bool> df)
{
  if (*df) { return; }
  warn << "logger[" << _pid << "] died\n";
  _cli = NULL;
  _x = NULL;
  _pid = -1;
}

//-----------------------------------------------------------------------

tamed void
sfs::logger_t::launch (evb_t ev, bool do_lock)
{
  tvars {
    str path;
    int t (0);
    vec<str> args;
  }

  if (do_lock) {
    twait { _lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
  }

  path = fix_exec_path ("sfs_logger");
  args.push_back (path);
  args.push_back ("-m");
  args.push_back (strbuf ("0%o", _mode));
  args.push_back (_file);

  do {
    if ((_x = axprt_unix_spawnv (path, args, 0x100000))) {
      _pid = axprt_unix_spawn_pid;
      _cli = aclnt::alloc (_x, logger_prog_1);
      _cli->seteofcb (wrap (this, &sfs::logger_t::eofcb, _destroyed));
    } else {
      twait { delaycb (1, 0, mkevent ()); }
    }
  } while (t < _tries && !_x);

  if (do_lock) {
    _lock.release ();
  }
  ev->trigger (_x);
}

//-----------------------------------------------------------------------

tamed void
sfs::logger_t::turn (evb_t ev)
{
  tvars {
    bool ret (false);
    clnt_stat err;
  }

  twait { _lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
  if (_cli) {
    twait { RPC::logger_prog_1::logger_turn (_cli, &ret, mkevent (err)); }
    if (err) {
      warn << "Error in logger::turn RPC: " << err << "\n";
    }
  }
  _lock.release ();
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
sfs::logger_t::log (str s, evb_t ev)
{
  tvars {
    bool ret (false);
    clnt_stat err;
  }

  twait { _lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }

  if (!_cli) {
    twait { launch (mkevent (ret), false); }
  }

  if (_cli) {
    twait { RPC::logger_prog_1::logger_log (_cli, s, &ret, mkevent (err)); }
    if (err) {
      warn << "Error in logger::log RPC: " << err << "\n";
    }
  }
  _lock.release ();
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

