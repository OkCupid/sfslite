// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"

//
// A test case to make sure that biggest callbacks work.
//

typedef ptr<callback<void,int,bool,str> > cb3_t ;

tamed static void callee3 (cb3_t cv)
{
  str s = "hello";
  TRIGGER(cv, 10, true, s);
}


tamed static void main_T (int argc, char **argv)
{
  tvars {
    int i;
    bool b;
    str s;
  }
  twait { callee3 (mkevent(i,b,s)); }
  exit (0);
}

int
main (int argc, char *argv[])
{
  main_T (argc, argv);
  amain ();
}
