// -*-c++-*-
/* $Id$ */

#include "async.h"
#include "qhash.h"

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


  qhash<int,int> x,y;
  x.insert (1, 5);
  y = x;

  qhash<int, int> z (x);
  
  warn << *y[1] << "\n";
  warn << *z[1] << "\n";
}
