
#include "async.h"
#include "cgc.h"

class foo_t {
public:
  foo_t (int a, int b) : _x (a+b) {}
  void bar () { warn << "foo_t() => " << _x << "\n"; }
private:
  int _x;
};

int
main (int argc, char *argv[])
{
  cgc::mmap_arena_t mmar (2);
  
  cgc::ptr<foo_t> x = cgc::allocator_t<foo_t> ().alloc (30, 40);
  cgc::ptr<foo_t> y = x;
  y->bar ();
  x->bar ();
  x = y = NULL;

}
