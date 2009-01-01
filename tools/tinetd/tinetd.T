
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "tinetd.h"
#include "tame_io.h"
#include "aapp.h"
#include "alog2.h"

#define EC_ERR -2
#define EC_INFO -1

extern char ** environ;

//=======================================================================

logger2_t logger;

//=======================================================================

cli_t::cli_t (child_t *s, int cli_fd, const sockaddr_in &sin, 
	      ptr<axprt_unix> x, ptr<aclnt> c)
  : _server (s),
    _cli_fd (cli_fd),
    _cli_addr (sin),
    _srv_x (x),
    _srv_cli (c)
{
  s->insert (this);
}

//-----------------------------------------------------------------------

cli_t::~cli_t ()
{
  if (_cli_fd >= 0)
    close (_cli_fd);
  _cli_fd = -1;

  _server->remove (this);
}

//-----------------------------------------------------------------------

tamed void
cli_t::run (evv_t ev)
{
  tvars {
    str n;
    aapp_newcon_t arg;
    aapp_status_t res;
    clnt_stat err;
    str a;
  } 

  n = _server->srvname ();
  sfs::x_host_addr::c2x (_cli_addr, &arg.addr);
  a = sfs::x_host_addr::x2s (arg.addr);

  logger.log (V_LO) << n << ": new connection from " << a << "\n";

  _srv_x->sendfd (_cli_fd);
  
  twait {
    RPC::aapp_server_prog_1::aapp_server_newcon 
      (_srv_cli, arg, &res, mkevent (err));
  }
  if (err) {
    warn << n << ": error in RPC connection for " << a << ": " << err << "\n";
  } else if (res != AAPP_OK) {
    warn << n << ": error in handoff for " << a << ": " << int (res) << "\n";
  } else {
    logger.log (V_LO) << n << ": end connection from " << a << "\n";
  }

  ev->trigger ();
}

//=======================================================================

child_t::child_t (main_t *m, port_t p, const vec<str> &v)
  : _main (m), 
    _port (p), 
    _cmd (v), 
    _state (NONE), 
    _lfd (-1),
    _pid (0) {}

//-----------------------------------------------------------------------

bool
child_t::init ()
{
  bool ret = true;
  _lfd = inetsocket (SOCK_STREAM, _port, _main->addr ().s_addr);
  if (_lfd > 0) {
    warn ("could not bind to port %d: %m\n", _port);
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
child_t::run ()
{
  listen (_lfd, 200);
  fdcb (_lfd, selread, wrap (this, &child_t::newcon));
  return true;
}

//-----------------------------------------------------------------------

tamed void
child_t::launch_loop ()
{
  twait { _poke_ev = mkevent (); }
  _poke_ev = NULL;

  while (true) {
    _x = NULL;
    launch ();
    if (_x) {
      while (_waiters.size ()) {
	evv_t::ptr w = _waiters.pop_front ();
	w->trigger ();
      }
      twait { wait_for_crash (mkevent ()); }
    }
    _x = NULL;
    twait { delaycb (_main->crash_wait (), 0, mkevent ()); }
  }
}

//-----------------------------------------------------------------------

void
child_t::launch ()
{
  const str &path = _cmd[0];

  logger.log (V_REG) << "starting up: " << path << "\n";
  ptr<axprt_unix> x = axprt_unix_aspawnv (path, _cmd, axprt::defps,
					  NULL, environ);
  if (x) {
    logger.log (V_REG) << "started: " << path << "; pid=" << _pid << "\n";
    _pid = axprt_unix_spawn_pid;
    _x = x;
    _cli = aclnt::alloc (_x, aapp_server_prog_1);
  }
}

//-----------------------------------------------------------------------

tamed void
child_t::wait_for_crash (evv_t ev)
{
  tvars {
    int rc;
  }
  twait { chldcb (_pid, mkevent (rc)); }
  logger.log (V_REG) << _cmd[0] << " died with exit code=" << rc << "\n";
}


//-----------------------------------------------------------------------

void
child_t::wait_for_it (evv_t ev)
{
  if (_poke_ev) { _poke_ev->trigger (); }
  if (_x) { ev->trigger (); }
  else { _waiters.push_back (ev); }
}

//-----------------------------------------------------------------------

tamed void
child_t::newcon_T ()
{
  tvars {
    sockaddr_in sin;
    socklen_t sinlen  (sizeof (sockaddr_in));
    int clifd;
    cli_t *cl;
    str addr;
  }

  bzero (&sin, sinlen);

  clifd = accept (_lfd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
  if (clifd < 0) {
    warn ("accept error: %m\n");
  } else {
   
    twait { tame::fdcb1 (clifd, selread, mkevent ()); }
    twait { wait_for_it (mkevent ()); }
    if (_x) {
      assert (_cli);
      cl = New cli_t (this, clifd, sin, _x, _cli);
      twait { cl->run (mkevent ()); }
      delete cl;
    } else {
      close (clifd);
      logger.log (V_LO) << ": rejecting connect from " << addr 
			<< " since server launch failed\n";
    }
  }
}

//=======================================================================

int 
main_t::config (int argc, char *argv[])
{
  int rc = 0;
  int ch;
  logger2_t::level_t level (V_REG);
  

  setprogname (argv[0]);

  _addr.s_addr = INADDR_ANY;
  _daemonize = false;
  _crash_wait = 10;

  while ((ch = getopt (argc, argv, "da:l:qvhw:")) != -1) {
    switch (ch) {
    case 'a': 
      {
	struct in_addr ia;
	if (inet_pton (AF_INET, optarg, static_cast<void *> (&ia)) != 1) {
	  warn << "cannot convert '" << optarg << "' to IP address\n";
	  usage ();
	  rc = EC_ERR;
	} else {
	  _addr = ia;
	}
      }
      break;
    case 'w':
      if (!convertint (optarg, &_crash_wait)) {
	warn << "cannot convert '" << optarg << "' to an int\n";
	usage ();
	rc = EC_ERR;
      }
      break;
    case 'd':
      _daemonize = true;
      break;
    case 'l':
      syslog_priority = optarg;
      break;
    case 'q':
      level = V_LO;
      break;
    case 'v':
      level = V_HI;
      break;
    case 'h':
      usage ();
      rc = EC_INFO;
      break;
    default:
      usage ();
      rc = EC_ERR;
      break;
    }
  }

  logger.set_level (level);

  argc -= optind;
  argv += optind;

  if (argc != 1 || !parse_config (argv[0])) {
    usage ();
    rc = EC_ERR;
  }

  return rc;
}

//-----------------------------------------------------------------------

bool
main_t::ch_apply (bool (child_t::*fn)() )
{
  hiter_t iter (_children);
  child_t *ch;

  bool ret = true;
  while ((ch = iter.next ())) {
    if ((ch->*fn)())
      ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool main_t::init () { return ch_apply (&child_t::init); }

//-----------------------------------------------------------------------

bool
main_t::run ()
{
  if (_daemonize) { daemonize (); }
  logger.log (V_REG) << "starting up; pid=" << getpid () << "\n";
  return ch_apply (&child_t::run);
}

//-----------------------------------------------------------------------

bool
main_t::insert (child_t *ch)
{
  bool ret = true;
  if (_children[ch->port ()]) {
    ret = false;
  } else {
    _children.insert (ch);
  }
  return ret;
}

//-----------------------------------------------------------------------

void
main_t::got_lazy_prox (vec<str> v, str loc, bool *errp)
{
  str cmd  = v.pop_front ();
  bool err = true;

  if (v.size () < 3) {
    warn << loc << ": usage: " << cmd << " <port> <cmd>\n";
  } else {
    str port_s = v.pop_front ();
    port_t port;
    if (!convertint (port_s, &port)) {
      warn << loc << ": cannot convert port to int (" << port_s << ")\n";
    } else {
      child_t *ch = New child_t (this, port, v);
      if (!insert (ch)) {
	warn << loc << ": duplicate child for port " << port << "\n";
      } else {
	err = false;
      }
    }
  }
  if (err) *errp = true;
}

//-----------------------------------------------------------------------

bool
main_t::parse_config (const str &f)
{
  conftab ct;
  ct.add ("LazyProx", wrap (this, &main_t::got_lazy_prox));
  return ct.run (f);
}

//-----------------------------------------------------------------------

void
main_t::usage ()
{
  warnx << "usage: " << progname << " [-qvh] <confile>\n";
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  main_t srv;
  int rc;

  if ((rc = srv.config (argc, argv)) != 0) return rc;
  if (!srv.init ()) return EC_ERR;
  if (!srv.run ()) return EC_ERR;

  amain ();
  return 0;
}

//-----------------------------------------------------------------------
