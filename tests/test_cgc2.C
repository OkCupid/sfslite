
#include "async.h"
#include "cgc2.h"

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
    cgc2::ptr<bar_t> x; 
    vec<cgc2::ptr<bar_t> > v;
    for (size_t i = 0; i < 4; i++) {
      cgc2::ptr<bar_t> x = cgc2::alloc<bar_t> (i);
      assert (x);
      v.push_back (x);
    }

    // Must clear v[0] before allocating new one, otherwise we're out of 
    // space.
    v[0] = NULL;
    x = cgc2::alloc<bar_t> (100);
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
  char _pad[31];
};

static void
test2(void) {

  cgc2::ptr<foo_t> x = cgc2::alloc<foo_t> (10,23);
  cgc2::ptr<foo_t> y = x;
  assert (y->baz () == 33);
  assert (x->baz () == 33);
  x = y = NULL;
  x = cgc2::alloc<foo_t> (40,50);

  vec<cgc2::ptr<foo_t> > v;
  for (int j = 0; j < 10; j++) {
    for (size_t i = 0; i < 100; i++) {
      x = cgc2::alloc<foo_t> (i, 0);
      assert (x);
      v.push_back (x);
    }
    v.clear ();
  }

  for (size_t i = 0; i < 100; i++) {
    v.push_back (cgc2::alloc<foo_t> (2*i, 0));
  }

  for (size_t i = 0; i < 20; i++) {
    v[i*5] = NULL;
  }

  for (size_t i = 0; i < 20; i++) {
    x = cgc2::alloc<foo_t> (i,300);
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

static void
test3()
{
  cgc2::ptr<bool> b = cgc2::alloc<bool> (true);
  *b = false;
  cgc2::ptr<char> c = cgc2::vecalloc<char,1000> ();
}

class wrobj_t : public cgc2::referee2_t<wrobj_t> 
{
public:
  wrobj_t (int i) : _i (i) {}
  void bar () { warn << "bar! " << _i << "\n"; }
private:
  int _i;
};

static void
test0 ()
{
  wrobj_t foo (10);
  cgc2::wkref2<wrobj_t> r (foo);
  r->bar ();
}


int
main (int argc, char *argv[])
{
  cgc2::std_cfg_t cfg;
  cfg._n_b_arenae = 2;
  cfg._size_b_arenae = 1;
  cfg._smallobj_lim = 0;
  cgc2::mgr_t::set (New cgc2::std_mgr_t (cfg));

  test0 ();

  for (int i = 0; i < 10; i++) {
    cgc2::mgr_t::get ()->sanity_check();
    test1();
    cgc2::mgr_t::get ()->sanity_check();
    test2();
    cgc2::mgr_t::get ()->sanity_check();
  }
  
  if (0) test3();
}

