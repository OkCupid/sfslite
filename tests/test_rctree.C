
#include "async.h"
#include "rctree.h"


struct dat_t {
  dat_t (const str &s) : _s (s) {}
  static ptr<dat_t> alloc (const str &s) { return New refcounted<dat_t> (s); }
  const str _s;
  void dump () { warn << "dump: " << _s << "\n"; }
  rctree_entry_t<dat_t> _lnk;
};

void
tree_test (void)
{
  rctree_t<int, dat_t, &dat_t::_lnk> tree;

  tree.insert (10, dat_t::alloc ("ten"));
  tree.insert (20, dat_t::alloc ("twenty"));
  tree.insert (30, dat_t::alloc ("thirty"));

  tree.root ()->dump ();
}

int
main (int argc, char *argv)
{
  tree_test ();
  


}
