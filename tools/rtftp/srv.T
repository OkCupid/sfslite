
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "rtftp_prot.h"
#include "sha1.h"
#include "rxx.h"
#include "rtftp.h"

//-----------------------------------------------------------------------

class srv_t {
public:
  srv_t () : _verbose (false), _port (RTFTP_TCP_PORT) {}
  int config (int argc, char *argv[]);
  bool run ();
private:
  bool _verbose;
  str _dir;
  int _port;
};

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-p <port>] <dir>\n";
}

//-----------------------------------------------------------------------

bool
srv_t::run ()
{
  bool rc = true;
  if (chdir (_dir.cstr ()) != 0) {
    warn ("cannot change to data directory %s: %m\n", _dir.cstr ());
    rc = false;
  }

  return rc;
  
}

//-----------------------------------------------------------------------

int
srv_t::config (int argc, char *argv[])
{
  int ch;
  int rc = 1;

  while ((ch = getopt (argc, argv, "v")) != -1) {
    switch (ch) {
    case 'v':
      _verbose = true;
      break;
    case 'p':
      if (!convertint (optarg, &_port)) {
	usage ();
	rc = -1;
	break;
      }
    case 'h':
      rc = 0;
      usage ();
      break;
    default:
      usage ();
      rc = -1;
      break;
    }
  }
  if (rc > 0) {
    argc -= optind;
    argv += optind;
    if (argc != 1) {
      usage ();
      rc = -1;
    } else {
      _dir = argv[0];
    }
  }
  return rc;
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  srv_t srv;
  int rc;
  setprogname (argv[0]);

  if ((rc = srv.config (argc, argv)) <= 0) {
    return rc;
  }

  srv.run ();
  amain ();
  return 0;
}

//-----------------------------------------------------------------------
