// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

//
// A test case to make sure that biggest callbacks work.
//

void callee3 (callback<void, int, bool, str>::ptr coordvar)
{
  SIGNAL (10, true, str ("hello"));
}

void callee4 (callback<void, int, bool, str, int>::ptr coordvar)
{
  SIGNAL (20, false, str ("goodbye"), 9);
}


int main_T (int argc, char *argv[])
{
  int i, j;
  bool b;
  str s;
  BLOCK { callee3 (@(i,b,s)); }
  BLOCK { callee4 (@(i,b,s,j)); }
  exit (0);
}

int
main (int argc, char *argv[])
{
  main_T (argc, argv);
  amin ();
}
