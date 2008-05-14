
#include "async.h"
#include "rctree.h"


struct dat_t {
  dat_t (const str &s) : _s (s) {}
  static ptr<dat_t> alloc (const str &s) { return New refcounted<dat_t> (s); }
  const str _s;
  void dump () { warn << "dump: " << _s << "\n"; }
  const str &to_str() const { return _s; }
  rctree_entry_t<dat_t> _lnk;
};

void
tree_test (void)
{
  rctree_t<int, dat_t, &dat_t::_lnk> tree;

  tree.insert (10, dat_t::alloc ("ten"));
  tree.insert (20, dat_t::alloc ("twenty"));
  tree.insert (30, dat_t::alloc ("thirty"));
  tree.insert (12, dat_t::alloc ("twelve"));
  tree.insert (4, dat_t::alloc ("four"));
  tree.insert (20, dat_t::alloc ("twenty(2)"));
  tree.insert (32, dat_t::alloc ("thirty-two"));

  ptr<dat_t> d = tree.root ();
  warn << "root: " << d->to_str () << "\n";
  for (d = tree.min_postorder (d); d; d = tree.next_postorder (d)) {
    warn << " pot: " << d->to_str () << "\n";
  }
 
}

int
main (int argc, char *argv)
{
  tree_test ();
  


}
