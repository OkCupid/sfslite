
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "tinetd.h"

//=======================================================================

child_t::child_t (port_t p, const vec<str> &v)
  : _port (p), _cmd (v), _state (NONE) {}

//=======================================================================

int 
main_t::config (int argc, char *argv[])
{
  int rc = 0;
  int ch;
  level_t level (V_REG);

  setprogname (argv[0]);

  while ((ch = getopt (argc, argv, "qvh")) != -1) {
    switch (ch) {
    case 'q':
      level = V_LO;
      break;
    case 'v':
      level = V_HI;
      break;
    case 'h':
      usage ();
      rc = -1;
      break;
    default:
      usage ();
      rc = -2;
      break;
    }
  }

  logger.set_level (level);

  argc -= optind;
  argv += optind;

  if (argc != 1) {
    usage ();
    rc = -2;
  }

  if (!parse_config (argv[0])) {
    rc = -2;
  }

  return rc;
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
      child_t *ch = New child_t (port, v);
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

bool
main_t::init ()
{
  return true;
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  main_t srv;
  int rc;

  if ((rc = srv.config (argc, argv)) != 0) return rc;

  logger.log (V_REG) << "starting up; pid=" << getpid () << "\n";

  if (!srv.init ()) return -1;

  amain ();
  return 0;
}
