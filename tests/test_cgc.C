
#include "async.h"
#include "cgc.h"

class bar_t {
public:
  bar_t (int a) : _x (a) {}
  int val () const { return _x; }
private:
  int _x;
  char _pad[1501];
};

static void
test1(void)
{
  for (size_t j = 0; j < 10; j++) {

    // Note; this x must be deallocated by the end of the loop, otherwise
    // it will hold a reference to an bar_t and we won't have spcae.
    // There's only enough room for exactly 4 of these things.
    cgc::ptr<bar_t> x; 
    vec<cgc::ptr<bar_t> > v;
    for (size_t i = 0; i < 4; i++) {
      cgc::ptr<bar_t> x = cgc::alloc<bar_t> (i);
      assert (x);
      v.push_back (x);
    }

    // Must clear v[0] before allocating new one, otherwise we're out of 
    // space.
    v[0] = NULL;
    x = cgc::alloc<bar_t> (100);
    assert (x);
    v[0] = x;

    for (int i = 1; i < 4; i++) {
      assert (v[i]->val () == i);
    }

    assert (v[0]->val () == 100);
  }
}

class foo_t {
public:
  foo_t (int a) : _x (a) {}
  foo_t (int a, int b) : _x (a+b) {}
  void bar () { warn << "foo_t() => " << _x << "\n"; }
  int baz () const { return _x; }
private:
  int _x;
  char _pad[37];
};

static void
test2(void) {

  cgc::ptr<foo_t> x = cgc::alloc<foo_t> (10,23);
  cgc::ptr<foo_t> y = x;
  assert (y->baz () == 33);
  assert (x->baz () == 33);
  x = y = NULL;
  x = cgc::alloc<foo_t> (40,50);

  vec<cgc::ptr<foo_t> > v;
  for (int j = 0; j < 10; j++) {
    for (size_t i = 0; i < 100; i++) {
      x = cgc::alloc<foo_t> (i, 0);
      assert (x);
      v.push_back (x);
    }
    v.clear ();
  }

  for (size_t i = 0; i < 100; i++) {
    v.push_back (cgc::alloc<foo_t> (2*i, 0));
  }

  for (size_t i = 0; i < 20; i++) {
    v[i*5] = NULL;
  }

  for (size_t i = 0; i < 20; i++) {
    x = cgc::alloc<foo_t> (i,300);
    assert (x);
    v[i*5] = x;
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
  cgc::mgr_t::get ()->sanity_check();

  test2();
}

