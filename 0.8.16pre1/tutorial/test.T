// -*-c++-*-
/* $Id$ */

#include "async.h"

int counter;
class A {
public:
  A () : x (counter++) { warn << "A(" << x << ")\n"; }
  A (const A &aa) : x (counter++) { warn << "A(" << x << ")\n"; }
  ~A () { warn << "~A(" << x << ");\n"; }
  int x;
};

void foofn (A a)
{

}

void bar ()
{
  warn << "x1\n";
  A a;
  warn << "x2\n";
  cbv cb = wrap (foofn, a);
  warn << "x3\n";
  //cb->signal ();
  (*cb)();
  warn << "x4\n";
}

int
main (int argc, char *argv[])
{
  bar ();
  warn << "x5\n";

}
