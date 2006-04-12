// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "async.h"

//
// A test case to make sure that biggest callbacks work.
//

typedef ptr<callback<void,int,bool,str> > cb3_t ;

TAMED static void callee3 (cb3_t parentcv)
{
  str s = "hello";
  SIGNAL (10, true, s);
}


TAMED static void main_T (int argc, char **argv)
{
  VARS {
    int i;
    bool b;
    str s;
  }
  BLOCK { callee3 (@(i,b,s)); }
  exit (0);
}

int
main (int argc, char *argv[])
{
  main_T (argc, argv);
  amain ();
}
