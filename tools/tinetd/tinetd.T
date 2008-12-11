
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "tinetd.h"

#define EC_ERR -2
#define EC_INFO -1

//=======================================================================

child_t::child_t (main_t *m, port_t p, const vec<str> &v)
  : _main (m), _port (p), _cmd (v), _state (NONE), _sfd (-1) {}

//-----------------------------------------------------------------------

bool
child_t::init ()
{
  bool ret = true;
  _sfd = inetsocket (SOCK_STREAM, _port, _main->addr ().s_addr);
  if (_sfd > 0) {
    warn ("could not bind to port %d: %m\n", _port);
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
child_t::run ()
{
  listen (_sfd, 200);
  fdcb (_sfd, selread, wrap (this, &child_t::newcon));
  return true;
}

//-----------------------------------------------------------------------

void
child_t::newcon ()
{

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

  while ((ch = getopt (argc, argv, "da:l:qvh")) != -1) {
    switch (ch) {
    case 'a': 
      {
	struct in_addr ia;
	if (inet_pton (AF_INET, optarg, static_cast<void *> (&ia)) != 1) {
	  warn << "cannot convert '" << optarg << "' to IP address\n";
	  rc = EC_ERR;
	} else {
	  _addr = ia;
	}
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
main_t::init ()
{
  hiter_t iter (_children);
  child_t *ch;
  bool ret = true;
  while ((ch = iter.next ())) {
    if (!ch->init ())
      ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
main_t::run ()
{
  if (_daemonize) { daemonize (); }

  logger.log (V_REG) << "starting up; pid=" << getpid () << "\n";
  

  hiter_t iter (_children);
  child_t *ch;
  bool ret = true;
  while ((ch = iter.next ())) {
    if (!ch->run ())
      ret = false;
  }
  return ret;
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
