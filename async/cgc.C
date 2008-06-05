
#include "cgc.h"
#include <sys/mman.h>

namespace cgc {

  //=======================================================================

  void bigslot_t::reseat () { _ptrslot->set_mem_slot (this); }

  //-----------------------------------------------------------------------
  
  void
  bigslot_t::copy (const bigslot_t *ms)
  {
    _ptrslot = ms->_ptrslot;
    memcpy (_data, ms->_data, ms->_sz);
    _sz = ms->_sz;
  }


  //=======================================================================

  static mgr_t *_g_mgr;

  //-----------------------------------------------------------------------

  static int cmp_fn (const memptr_t *mp, const arena_t *a)
  {
    return a->cmp (mp);
  }

  //-----------------------------------------------------------------------

  redirect_ptr_t *
  mgr_t::aalloc (size_t sz)
  {
    arena_t *a = pick (sz);
    if (a) return a->aalloc (sz);
    else return NULL;
  }

  //-----------------------------------------------------------------------

  arena_t *
  mgr_t::lookup (const memptr_t *p)
  {
    return _tree.search (wrap (cmp_fn, p));
  }

  //-----------------------------------------------------------------------

  void
  mgr_t::insert (arena_t *a)
  {
    _tree.insert (a);
  }

  //-----------------------------------------------------------------------
  
  void
  mgr_t::remove (arena_t *a)
  {
    _tree.remove (a);
  }

  //-----------------------------------------------------------------------

  mgr_t *
  mgr_t::get () 
  {
    if (!_g_mgr) {
      _g_mgr = New std_mgr_t (std_cfg_t ());
    }
    return _g_mgr;
  }

  //-----------------------------------------------------------------------

  void
  mgr_t::set (mgr_t *m)
  {
    assert (!_g_mgr);
    _g_mgr = m;
  }

  //=======================================================================

  arena_t *
  std_mgr_t::pick (size_t sz)
  {
    bigobj_arena_t *s, *p;

    s = _next_big;
    if (!s) s = _bigs.first;
    p = s;

    bool go = true;
    bigobj_arena_t *ret = NULL;
    
    while (p && go && !ret) {
      if (p->can_fit (sz)) {
	ret = p;
      } else {
	p = _bigs.next (p);
	if (!p) p = _bigs.first;
	if (s == p) {
	  go = false;
	}
      }
    }

    _next_big = p;
    return ret;
  }

  //-----------------------------------------------------------------------
  
  std_mgr_t::std_mgr_t (const std_cfg_t &cfg)
    : _cfg (cfg),
      _next_big (NULL)
  {
    for (size_t i = 0; i < _cfg._n_b_arenae; i++) {
      mmap_bigobj_arena_t *a = New mmap_bigobj_arena_t (_cfg._size_b_arenae);
      mgr_t::insert (a);
      _bigs.insert_tail (a);
    }
  }

  //=======================================================================

  //-----------------------------------------------------------------------

  bigptr_t *
  bigobj_arena_t::get_free_ptrslot ()
  {
    bigptr_t *ret = NULL;
    if (_free_ptrslots.size ()) {
      ret = _free_ptrslots.pop_back ();
    } else {
     ret = reinterpret_cast<bigptr_t *> (_nxt_ptrslot);
     _nxt_ptrslot += sizeof (*ret);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

#define ALIGN(x,z) (((x) + (z) -1) & ~(z-1))
#define BOA_OBJ_ALIGN(x) (ALIGN(x, sizeof (void *)))

  //-----------------------------------------------------------------------

  bool
  bigobj_arena_t::can_fit (size_t sz) const
  {
    sz = BOA_OBJ_ALIGN(sz);
    return (bigslot_t::size (sz) + _nxt_memslot <= _nxt_ptrslot);
  }
  
  //-----------------------------------------------------------------------

  redirect_ptr_t *
  bigobj_arena_t::aalloc (size_t sz)
  {
    bigptr_t *res = NULL;
    if (can_fit (sz)) {
      bigslot_t *ms = reinterpret_cast<bigslot_t *> (_nxt_memslot);
      res = new (_nxt_ptrslot) bigptr_t (ms);

      sz = BOA_OBJ_ALIGN(sz);
      
      ms->_sz = sz;
      ms->_ptrslot = res;

      _nxt_memslot += ms->size ();
      _nxt_ptrslot -= sizeof (*res);
      _memslots.insert_tail (ms);
    }
    return res;
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::collect_ptrslots (void)
  {
    bigptr_t *p = reinterpret_cast<bigptr_t *> (_top) - 1;
    bigptr_t *last = reinterpret_cast<bigptr_t *> (_nxt_ptrslot) + 1;
    bigptr_t *last_used = NULL;

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
  bigobj_arena_t::compact_memslots (void)
  {
    memptr_t *p = _base;
    bigslot_t *m = _memslots.first;
    bigslot_t *n = NULL;

    memslot_list_t nl;
    
    while (m) {
      n = _memslots.next (m);
      _memslots.remove (m);
      bigslot_t *ns = reinterpret_cast<bigslot_t *> (p);
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
  bigobj_arena_t::gc (void)
  {
    collect_ptrslots ();
    compact_memslots ();
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::init ()
  {
    _top = _base + _sz;
    _nxt_memslot = _base;
    _nxt_ptrslot = _top - sizeof (bigptr_t);
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

  size_t mmap_bigobj_arena_t::pagesz;

  //-----------------------------------------------------------------------

  mmap_bigobj_arena_t::mmap_bigobj_arena_t (size_t sz)
  {
    if (!pagesz) {
      pagesz = sysconf (_SC_PAGE_SIZE);
    }

    sz = ALIGN(sz, pagesz);

    void *v = mmap (NULL, sz, PROT_READ | PROT_WRITE, 
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (!v) 
      panic ("mmap failed: %m\n");

    _base = static_cast<memptr_t *> (v);
    _sz = sz;
    init ();
  }

  //-----------------------------------------------------------------------

  mmap_bigobj_arena_t::~mmap_bigobj_arena_t ()
  {
    munmap (_base, _sz);
  }

  //=======================================================================

};
