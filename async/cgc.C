
#include "cgc.h"
#include <sys/mman.h>

namespace cgc {

  //=======================================================================

  void memslot_t::reseat () { _ptrslot->set_mem_slot (this); }

  //-----------------------------------------------------------------------
  
  void
  memslot_t::copy (const memslot_t *ms)
  {
    _ptrslot = ms->_ptrslot;
    memcpy (_data, ms->_data, ms->_sz);
    _sz = ms->_sz;
  }


  //=======================================================================
  
  arena_set_t g_arena_set;

  //-----------------------------------------------------------------------

  static int cmp_fn (const memptr_t *mp, const arena_t *a)
  {
    return a->cmp (mp);
  }

  //-----------------------------------------------------------------------

  arena_t *
  arena_set_t::v_lookup (const memptr_t *p)
  {
    return _tree.search (wrap (cmp_fn, p));
  }

  //-----------------------------------------------------------------------

  void
  arena_set_t::insert (arena_t *a)
  {
    _order.insert_tail (a);
    _tree.insert (a);
  }

  //-----------------------------------------------------------------------
  
  void
  arena_set_t::remove (arena_t *a)
  {
    _order.remove (a);
    _tree.remove (a);
  }

  //-----------------------------------------------------------------------

  arena_t *
  arena_set_t::pick (size_t sz)
  {
    if (!_p) _p = _order.first;
    if (!_p) return NULL;
    arena_t *ret = _p;
    _p = _order.next (_p);
    return ret;
  }

  //=======================================================================

  //-----------------------------------------------------------------------

  untyped_bigptr_t *
  arena_t::get_free_ptrslot ()
  {
    untyped_bigptr_t *ret = NULL;
    if (_free_ptrslots.size ()) {
      ret = _free_ptrslots.pop_back ();
    } else {
     ret = reinterpret_cast<untyped_bigptr_t *> (_nxt_ptrslot);
     _nxt_ptrslot += sizeof (*ret);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

#define ALIGNSZ (sizeof(void *))
#define ALIGN(x) \
  (((x) + ALIGNSZ) & ~ALIGNSZ)

  untyped_bigptr_t *
  arena_t::alloc (size_t sz)
  {
    untyped_bigptr_t *res = NULL;
    if (memslot_t::size (sz) + _nxt_memslot <= _nxt_ptrslot) {
      memslot_t *ms = reinterpret_cast<memslot_t *> (_nxt_memslot);
      bigptr_t<int> *bp = reinterpret_cast<bigptr_t<int> *> (_nxt_ptrslot);
      untyped_bigptr_t *res = bp;

      sz = ALIGN(sz);
      
      ms->_sz = sz;
      ms->_ptrslot = res;

      res->init (ms, 1);

      _nxt_memslot += ms->size ();
      _nxt_ptrslot += sizeof (*bp);
      _memslots.insert_tail (ms);
    }
    return res;
  }

  //-----------------------------------------------------------------------

  void
  arena_t::collect_ptrslots (void)
  {
    untyped_bigptr_t *p = reinterpret_cast<untyped_bigptr_t *> (_top) - 1;
    untyped_bigptr_t *last = 
      reinterpret_cast<untyped_bigptr_t *> (_nxt_ptrslot) + 1;
    untyped_bigptr_t *last_used = NULL;

    for ( ; p >= last; p--) {
      if (p->count () == 0) {
	p->mark_free ();
	_free_ptrslots.push_back (p);
      } else {
	last_used = p;
      }
    }

    if (last_used > last) {
      last = last_used;
      _nxt_ptrslot = reinterpret_cast<memptr_t *> (last - 1);
      while (_free_ptrslots.size () && _free_ptrslots.back () > last)
	_free_ptrslots.pop_back ();
    }
  }

  //-----------------------------------------------------------------------

  void
  arena_t::compact_memslots (void)
  {
    memptr_t *p = _base;
    memslot_t *m = _memslots.first;
    memslot_t *n = NULL;

    memslot_list_t nl;
    
    while (m) {
      n = _memslots.next (m);
      _memslots.remove (m);
      memslot_t *ns = reinterpret_cast<memslot_t *> (p);
      if (m->v_data () > p) {
	ns->copy (m);
	ns->reseat ();
	p += ns->size ();
      }
      nl.insert_tail (ns);
      m = n;
    }

    _memslots.first = nl.first;
    _memslots.plast = nl.plast;

    _nxt_memslot = p;
  }

  //-----------------------------------------------------------------------

  void
  arena_t::gc (void)
  {
    collect_ptrslots ();
    compact_memslots ();
  }

  //-----------------------------------------------------------------------

  void
  arena_t::init (memptr_t *base, u_int sz)
  {
    _base = base;
    _top = _base + sz;
    _nxt_ptrslot = _base;
    _nxt_memslot = _top - sizeof (memslot_t);
    _sz = sz;
    
    /*
      assert (sizeof(size_t) == sizeof (memptr_t *));
      _key = reinterpret_cast<size_t> (_base);
    */

    g_arena_set.insert (this);
  }

  //-----------------------------------------------------------------------

  arena_t::~arena_t ()
  {
    g_arena_set.remove (this);
  }

  //-----------------------------------------------------------------------

  int
  arena_t::cmp (const memptr_t *mp) const
  {
    if (mp < _base) return -1;
    else if (mp >= _base + _sz) return 1;
    else return 0;
  }

  //=======================================================================

  size_t mmap_arena_t::pagesz;

  //-----------------------------------------------------------------------

  mmap_arena_t::mmap_arena_t (size_t npages)
  {
    if (!pagesz) {
      pagesz = sysconf (_SC_PAGE_SIZE);
    }
    size_t sz = npages * pagesz;

    void *v = mmap (NULL, sz, PROT_READ | PROT_WRITE, 
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (!v) 
      panic ("mmap failed: %m\n");

    init (static_cast<memptr_t *> (v), sz);
  }

  //-----------------------------------------------------------------------

  mmap_arena_t::~mmap_arena_t ()
  {
    munmap (_base, _sz);
  }

  //-----------------------------------------------------------------------

  arena_t *
  arena_t::pick_arena (size_t sz)
  {
    return g_arena_set.pick (sz);
  }

  //-----------------------------------------------------------------------

  //=======================================================================

};
