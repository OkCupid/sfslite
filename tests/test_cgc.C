
#include "async.h"
#include "cgc.h"

class bar_t {
public:
  bar_t (int a) : _x (a) {}
private:
  int _x;
  char _pad[1501];
};

static void
test1(void)
{
  vec<cgc::ptr<bar_t> > v;
  for (size_t i = 0; i < 4; i++) {
    v.push_back (cgc::alloc<bar_t> (i));
  }
  v[0] = NULL;
  v[0] = cgc::alloc<bar_t> (100);
}

class foo_t {
public:
  foo_t (int a) : _x (a) {}
  foo_t (int a, int b) : _x (a+b) {}
  void bar () { warn << "foo_t() => " << _x << "\n"; }
  int baz () const { return _x; }
private:
  int _x;
  char _pad[51];
};

static void
test2(void) {
  
  cgc::ptr<foo_t> x = cgc::alloc<foo_t> (10,23);
  cgc::ptr<foo_t> y = x;
  y->bar ();
  x->bar ();
  x = y = NULL;
  x = cgc::alloc<foo_t> (40,50);

  vec<cgc::ptr<foo_t> > v;
  for (size_t i = 0; i < 100; i++) {
    v.push_back (cgc::alloc<foo_t> (i, 0));
  }
  v[0]->bar ();
  for (size_t i = 0; i < 100; i++) {
    v.push_back (cgc::alloc<foo_t> (2*i, 0));
  }
  v[0]->bar ();

  for (size_t i = 0; i < 20; i++) {
    v[i*5] = NULL;
  }

  for (size_t i = 0; i < 20; i++) {
    v[i*5] = cgc::alloc<foo_t> (i,300);
  }

  for (size_t i = 0; i < 100; i++) {
    size_t x = v[i]->baz ();
    if (i % 5 == 0) {
      assert (x == 300 + i/5);
    } else {
      assert (x == 2*i);
    }
  }
}

int
main (int argc, char *argv[])
{
  cgc::std_cfg_t cfg;
  cfg._n_b_arenae = 2;
  cfg._size_b_arenae = 1;
  cgc::mgr_t::set (New cgc::std_mgr_t (cfg));

  test1();
  test2();
}

