
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "tame_io.h"

//-----------------------------------------------------------------------

class inetd_main_t {
public:
  inetd_main_t () {}
  int config (int argc, char *argv[]);
  bool init ();
private:

};

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

int 
inetd_main_t::config (int argc, char *argv[])
{
  int rc = 0;
  return rc;
}

//-----------------------------------------------------------------------

bool
inetd_main_t::init ()
{
  return true;
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  inetd_main_t srv;
  int rc;

  setprogname (argv[0]);

  if ((rc = srv.config (argc, argv)) != 0) return rc;
  if (!srv.init ()) return -1;

  amain ();
  return 0;
}
