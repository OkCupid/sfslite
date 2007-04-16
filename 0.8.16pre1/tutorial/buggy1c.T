// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "arpc.h"

bool fix_bug;

/*
 * buggy1c
 *
 *   A test program that should complain about Bug #1b -- a closure
 *   leaked past the end of a function's control flow, from a function
 *   that uses NONBLOCK...
 */

static void finish (bool rc)
{
  warn << "called finish\n";
  delaycb (2, 0, wrap (exit, rc ? 0 : -1));
}

TAMED static void 
buggy1c (cbb cv)
{
  VARS { 
    coordgroup_t<> JG (__FILE__, __LINE__); 
    int i;
  }

  delaycb (1, 0, @[JG]());
  delaycb (2, 0, @[JG]());


  for (i = 0; i < (fix_bug ? 2 : 1); i++) {
    WAIT (JG);
    if (i == 0) 
      SIGNAL (cv, true);
  }
  warn << "leaving buggy1c\n";
}

static void usage ()
{
  fatal << "usage: " << progname << " [-f]\n";
}
 
int main (int argc, char *argv[])
{
  int ch;
  fix_bug = false;
  setprogname (argv[0]);
  while ((ch = getopt (argc, argv, "f")) != -1) {
    switch (ch) {
    case 'f':
      fix_bug = true;
      break;
    default:
      usage ();
    }
  }
  buggy1c (wrap (finish));
  amain ();
}
