// -*-c++-*-
/* $Id: ex12.T 2548 2007-03-05 02:02:48Z max $ */

#include "tame.h"
#include "parseopt.h"

tamed static void
c (cbv ev)
{
  warn << "+c\n";
  twait { delaycb (1, 0, mkevent ()); }
  warn << "-c\n";
  ev->trigger ();
}

tamed static void
b (cbv ev)
{
  warn << "+b\n";
  twait { c (mkevent ()); }
  warn << "-b\n";
  ev->trigger ();
}

tamed static void
a (cbv ev)
{
  warn << "+a\n";
  twait { b (mkevent ()); }
  warn << "-a\n";
  ev->trigger ();
}

tamed static void
main2 ()
{
  twait { a( mkevent ()); }
  exit (0);
}

int main(int argc, char *argv[])
{
  main2 ();
  amain ();
}
