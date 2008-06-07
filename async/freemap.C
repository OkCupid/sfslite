
#include "freemap.h"


node_t::node_t (u_int32_t i) : _id (i), _bits (0) {}

bool
node_t::getbit (u_int i) const
{
  assert (i < n_bits);
  return (_bits & (1 << i));
}

void
node_t::setbit (u_int i, bool b) const
{
  if (b) {
    _bits = _bits | (1 << i);
  } else {
    _bits = _bits & (~(1 << i));
  }
}

bool
node_t::is_empty () const
{
  return (_bits == 0);
}

int
freemap_t::alloc () 
{
  int ret;
  node_t *n = root ();
  if (!n) {
    ret = -1;
  } else {
    ret = n->topbit ();
    assert (ret >= 0);
    n->setbit (ret, false);
    if (n->is_empty ()) {
      clobber_root ();
    }
  }
  return ret;
}

void
freemap_t::dealloc (u_int i)
{
  


}

void
freemap_t::clobber_root ()
{
  if (_cursor == 1) {
    _cursor = 0;
  } else {
    swap (0, --_cursor);
    push_down ();
  }
}

void
freemap_t::push_down ()
{
  assert (_cursor > 0);
  int next = 0;
  u_int32_t k = getkey (0);
  for (int i = 0; next >= 0; i = next) {
    
    int l = left (i);
    int r = right (r);
    int smallest = i;

    if (l >= 0 && cmp (ind2key (l), k) < 0)
      smallest = l;
    if (r >= 0 && cmp (ind2key (r), ind2key (smallest)) < 0)
      smallest = r;

    if (smallest != i) {
      swap (smallest, i);
      next = smallest;
    } else {
      next = -1;
    }
  }
}

void
freemap_t::swap (size_t i, size_t j)
{
  assert (i < _cursor);
  assert (j < _cursor);
  node_t n = _nodes[i];
  _nodes[i] = _nodes[j];
  _nodes[j] = n;
}

void
freemap_t::ind2key (size_t i) const
{
  assert (i < _cursor);
  return _nodes[i]._id;
}

int
node_t::topbit () const
{
  int ret = -1;
  if (!is_empty ()) {
    for (int i = n_bits - 1; ret < 0 && i >= 0; i--) {
      if (getbit (i)) 
	ret = _id * n_bits + i;
    }
  }
  return ret;
}
