
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"

//-----------------------------------------------------------------------

class cli_t {
public:
  cli_t () {}
  int config (int argc, const char *const *argv);
  void run (CLOSURE);
  static void usage ();
private:

};

//-----------------------------------------------------------------------

int 
cli_t::config (int argc, const char *const *argv)
{
  int rc = 1;
  while ((ch = getopt (argc, argv, "vh"))) {
    switch (ch) {
    case 'v':
      _verbse = true;
      break;
    case 'h':
      usage ();
      rc = 0;
      break;
    default:
      usage ();
      rc = -1;
    }
  }
  return rc;
}



//-----------------------------------------------------------------------

int 
main (int argc, char *argv[])
{
  cli_t cli;
  int rc;
  setprogname (argv[0]);

  if ((rc = cli.config (argc, argv)) <= 0) {
    return rc;
  }

  cli.run ();
  amain ();

  return 0; // never get here
}
