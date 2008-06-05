
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
  cgc::std_cfg_t cfg;
  cfg._n_b_arenae = 2;
  cfg._size_b_arenae = 1;

  cgc::mgr_t::set (New cgc::std_mgr_t (cfg));
  
  cgc::ptr<foo_t> x = cgc::alloc<foo_t> (30, 40);
  cgc::ptr<foo_t> y = x;
  y->bar ();
  x->bar ();
  x = y = NULL;

}
