
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "tinetd.h"
#include "tame_io.h"

#define EC_ERR -2
#define EC_INFO -1

//=======================================================================

cli_t::cli_t (child_t *s, int cli_fd, const str &cli_addr, int srv_fd)
  : _server (s),
    _cli_fd (cli_fd),
    _cli_addr (cli_addr),
    _srv_fd (srv_fd)
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
    int srvfd;
    rendezvous_t<> rv (__FILE__, __LINE__);
    ptr<tame::std_proxy_t> cli2srv (New refcounted<tame::std_proxy_t> ());
    ptr<tame::std_proxy_t> srv2cli (New refcounted<tame::std_proxy_t> ());
    str n;
  } 

  n = _server->srvname ();

  logger.log (V_LO) << n << ": new connection from " << _cli_addr << "\n";

  cli2srv->go (_cli_fd, _srv_fd, mkevent (rv));
  srv2cli->go (_srv_fd, _cli_fd, mkevent (rv));

  twait (rv);

  logger.log (V_LO) << n << ": end connection from " << _cli_addr << "\n";
  ev->trigger ();

}

//=======================================================================

child_t::child_t (main_t *m, port_t p, const vec<str> &v)
  : _main (m), 
    _port (p), 
    _cmd (v), 
    _state (NONE), 
    _lfd (-1),
    _srv_fd (-1),
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
    _srv_fd = -1;
    twait { launch (mkevent ()); }
    if (_srv_fd < 0) {
      twait { delaycb (_main->crash_wait (), 0, mkevent ()); }
    } else {
      while (_waiters.size ()) {
	evi_t::ptr w = _waiters.pop_front ();
	w->trigger (_srv_fd);
      }
      twait { wait_for_crash (mkevent ()); }
      _srv_fd = -1;
    }
  }
}

//-----------------------------------------------------------------------

tamed void
child_t::launch (evv_t ev)
{

}

//-----------------------------------------------------------------------

tamed void
child_t::wait_for_crash (evv_t ev)
{

}


//-----------------------------------------------------------------------

void
child_t::get_srv_fd (evi_t ev)
{
  if (_poke_ev) { _poke_ev->trigger (); }
  if (_srv_fd >= 0) { ev->trigger (_srv_fd); }
  else { _waiters.push_back (ev); }
}

//-----------------------------------------------------------------------

tamed void
child_t::newcon_T ()
{
  tvars {
    sockaddr_in sin;
    socklen_t sinlen  (sizeof (sockaddr_in));
    int clifd, srvfd;
    cli_t *cl;
    str addr;
  }

  bzero (&sin, sinlen);

  clifd = accept (_lfd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
  if (clifd < 0) {
    warn ("accept error: %m\n");
  } else {
   
    {
      strbuf b;
      b << inet_ntoa (sin.sin_addr) << ":" << htons (sin.sin_port);
      addr = b;
    }

    twait { tame::fdcb1 (clifd, selread, mkevent ()); }
    twait { get_srv_fd (mkevent (srvfd)); }
    if (srvfd >= 0) {
      cl = New cli_t (this, clifd, addr, srvfd);
      twait { cl->run (mkevent ()); }
      delete cl;
    } else {
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
  level_t level (V_REG);
  

  setprogname (argv[0]);

  _addr.s_addr = INADDR_ANY;
  _daemonize = false;

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
  bool ret;
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
