
#include "cgc.h"
#include <sys/mman.h>
#include <async.h>

namespace cgc {

  //=======================================================================

  static size_t pagesz;

  static size_t get_pagesz ()
  {
    if (!pagesz) {
      pagesz = sysconf (_SC_PAGE_SIZE);
    }
    return pagesz;
  }

  //=======================================================================

  void bigslot_t::reseat () { check(); _ptrslot->set_mem_slot (this); }

  //-----------------------------------------------------------------------
  
  void
  bigslot_t::copy_reinit (const bigslot_t *ms)
  {
    if (debug_warnings)
      warn ("copy data from %p to %p (%u bytes)\n", ms->_data, _data, ms->_sz);
    _ptrslot = ms->_ptrslot;
    memcpy (_data, ms->_data, ms->_sz);
    _sz = ms->_sz;
    debug_init ();
  }

  //-----------------------------------------------------------------------

  void
  bigslot_t::deallocate (bigobj_arena_t *a)
  {
    check ();
    a->remove (this);
    mark_deallocated (this, size ());
  }

  //-----------------------------------------------------------------------

  static inline size_t
  align (size_t in, size_t a)
  {
    a--;
    return (in + a) & ~a;
  }

  //-----------------------------------------------------------------------

  static inline size_t
  boa_obj_align (size_t sz)
  {
    return align (sz, sizeof (void *));
  }

  //-----------------------------------------------------------------------

  size_t
  bigslot_t::size (size_t s)
  {
    return boa_obj_align (s) + sizeof (bigslot_t);
  }

  //=======================================================================

  static mgr_t *_g_mgr;

  //-----------------------------------------------------------------------

  static int cmp_fn (const memptr_t *mp, const arena_t *a)
  {
    return a->cmp (mp);
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

  template<class L, class O>
  class cyclic_list_iterator_t {
  public:
    cyclic_list_iterator_t (L *l, O *s) 
      : _list (l), _start (s ? s : l->first), _p (_start) {}
    
    O *next ()
    {
      O *ret = _p;
      if (_p) {
	_p = _list->next (_p);
	if (!_p) _p = _list->first;
	if (_p == _start) _p  = NULL;
      }
      return ret;
    }

  private:
    L *_list;
    O *_start, *_p;
  };


  //=======================================================================

  void
  std_mgr_t::became_vacant (smallobj_arena_t *a, int soa_index)
  {
    _smalls[soa_index]->became_vacant (a);
  }

  //-----------------------------------------------------------------------

  bigobj_arena_t *
  std_mgr_t::big_pick (size_t sz)
  {
    cyclic_list_iterator_t<boa_list_t, bigobj_arena_t> it (&_bigs, _next_big);
    bigobj_arena_t *p;

    while ((p = it.next ()) && !(p->can_fit (sz))) {}

    if (p) {
      _next_big = p;
    } else {
      if (debug_mem)
	sanity_check ();
      p = gc_make_room_big (sz);
      if (debug_mem)
	sanity_check ();
    }
    return p;
  }

  //-----------------------------------------------------------------------

  redirector_t 
  std_mgr_t::big_alloc (size_t sz)
  {
    bigobj_arena_t *a = big_pick (sz);
    if (a) return a->aalloc (sz);
    else return redirector_t ();
  }

  //-----------------------------------------------------------------------

  void
  std_mgr_t::sanity_check (void) const
  {
    for (bigobj_arena_t *a = _bigs.first; a; a = _bigs.next (a)) {
      a->sanity_check ();
    }
  }

  //-----------------------------------------------------------------------

  void
  std_mgr_t::gc (void)
  {
    for (bigobj_arena_t *a = _bigs.first; a; a = _bigs.next (a)) {
      a->gc ();
    }
  }

  //-----------------------------------------------------------------------

  redirector_t
  std_mgr_t::aalloc (size_t sz)
  {
    if (sz < _smallobj_lim) return small_alloc (sz);
    else return big_alloc (sz);
  }

  //-----------------------------------------------------------------------

  redirector_t
  std_mgr_t::small_alloc (size_t sz)
  {
    int i;
    size_t roundup = _sizer.find (sz, &i);
    assert (roundup != 0);
    assert (i >= 0);

    redirector_t ret = _smalls[i]->aalloc (sz);
    if (!ret) {
      smallobj_arena_t *a = alloc_soa (roundup, i);
      ret = a->aalloc (sz);
      assert (ret);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  smallobj_arena_t *
  std_mgr_t::alloc_soa (size_t sz, int ind)
  {
    int n = ( 100 * ( sizeof (smallobj_arena_t) +
		      freemap_t::fixed_overhead () )) / 
      (get_pagesz () * _cfg._smallobj_max_overhead_pct) + 1;

    smallobj_arena_t *a = 
      New mmap_smallobj_arena_t (n * get_pagesz (), 
				 _sizer.ind2size(ind-1) + 1,
				 sz, 
				 this, 
				 ind);
    mgr_t::insert (a);
    _smalls[ind]->add (a);

    return a;
  }

  //-----------------------------------------------------------------------

  void
  std_mgr_t::report (void) const
  {
    warn << "CGC Memory report-------------------\n";
    for (bigobj_arena_t *a = _bigs.first; a; a = _bigs.next (a)) {
      a->report ();
    }
  }

  //-----------------------------------------------------------------------

  bigobj_arena_t *
  std_mgr_t::gc_make_room_big (size_t sz)
  {
    cyclic_list_iterator_t<boa_list_t, bigobj_arena_t> it (&_bigs, _next_big);
    bigobj_arena_t *p;
    sz = bigslot_t::size (sz);

    while ((p = it.next ()) && !(p->gc_make_room (sz))) {}
    if (p) _next_big = p;
    return p;
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
    
    size_t tmp = _cfg._smallobj_lim;
    if (tmp == 0) {
      tmp = bigslot_t::size (0) + sizeof (bigptr_t);
      tmp *= 2;
    }
    int ind;
    _smallobj_lim = _sizer.find (tmp, &ind);
    assert (ind >= 0);
    assert (_smallobj_lim);

    for (int i = 0; i < ind; i++) {
      _smalls.push_back (New soa_cluster_t (_sizer.ind2size (i)));
    }
  }

  //=======================================================================

  //-----------------------------------------------------------------------

  bigptr_t *
  bigobj_arena_t::get_free_ptrslot ()
  {
    bigptr_t *ret = NULL;
    bigptr_t *nxt = reinterpret_cast<bigptr_t *> (_nxt_ptrslot);
    if (_free_ptrslots.n_elem ()) {
      ret = _free_ptrslots.pop_back ();
      assert (ret->count () == -1);
      assert (ret > nxt);
    } else {
      ret = nxt--;
      _nxt_ptrslot = reinterpret_cast<memptr_t *> (nxt);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void bigobj_arena_t::mark_free (bigptr_t *p) {}

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::sanity_check (void) const
  {
    for (bigslot_t *s = _memslots->first; s; s = _memslots->next (s)) {
      s->check ();
    }

    bigptr_t *bottom = reinterpret_cast<bigptr_t *> (_nxt_ptrslot) + 1;
    bigptr_t *top = reinterpret_cast<bigptr_t *> (_top);

    if (_free_ptrslots.n_elem ()) {
      assert (_free_ptrslots.back () >= bottom);
    }

    for (bigptr_t *p = bottom; p < top; p++) {
      p->check ();
    }
    
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::report (void) const
  {
    size_t sz = 0;
    for (bigslot_t *s = _memslots->first; s; s = _memslots->next (s)) {
      sz += s->size ();
    }

    warn (" bigobj_arena(%p -> %p): %zd in objs; %zd free; %zd unclaimed; "
	  "%zd ptrslots; slotp=%p; ptrp=%p\n",
	  _base, _top, 
	  sz,
	  free_space (), _unclaimed_space, 
	  _free_ptrslots.n_elem(),
	  _nxt_memslot,
	  _nxt_ptrslot);
  }

  //-----------------------------------------------------------------------

  size_t
  bigobj_arena_t::free_space () const
  {
    if (_nxt_ptrslot > _nxt_memslot) 
      return (_nxt_ptrslot - _nxt_memslot);
    else
      return 0;
  }

  //-----------------------------------------------------------------------

  bool
  bigobj_arena_t::gc_make_room (size_t sz)
  {
    bool ret = false;
    if (sz <= _unclaimed_space  + free_space ()) {
      gc ();
      ret = true;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  bigobj_arena_t::can_fit (size_t sz) const
  {
    sz = boa_obj_align (sz);
    return (bigslot_t::size (sz) <= free_space ());
  }
  
  //-----------------------------------------------------------------------

  redirector_t
  bigobj_arena_t::aalloc (size_t sz)
  {
    redirector_t res;
    if (can_fit (sz)) {

      assert (_nxt_memslot < _nxt_ptrslot);
      
      bigslot_t *ms_tmp = reinterpret_cast<bigslot_t *> (_nxt_memslot);
      bigptr_t *p_tmp = get_free_ptrslot ();
      assert (p_tmp);
      bigptr_t *p = new (p_tmp) bigptr_t (ms_tmp);
      sz = boa_obj_align (sz);
      bigslot_t *ms = new (_nxt_memslot) bigslot_t (sz, p);
      assert (ms == ms_tmp);
      assert (p->count () == 0);

      if (debug_warnings)
	warn ("allocated %p -> %p\n", ms, ms->_data + sz);

      _nxt_memslot += ms->size ();
      _memslots->insert_tail (ms);
      res.init (p);
    }
    return res;
  }

  //-----------------------------------------------------------------------

  bigslot_t::bigslot_t (size_t sz, bigptr_t *p)
    : _sz (sz), _ptrslot (p) 
  { 
    debug_init(); 
    mark_unitialized (_data, _sz);
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::collect_ptrslots (void)
  {
    bigptr_t *p = reinterpret_cast<bigptr_t *> (_top) - 1;
    bigptr_t *bottom = reinterpret_cast<bigptr_t *> (_nxt_ptrslot);
    bigptr_t *last = NULL;

    _free_ptrslots.clear ();

    for ( ; p > bottom; p--) {
      p->check ();
      if (p->count () == -1) {
	_free_ptrslots.push_back (p);
      } 
      last = p;
    }
    if (last)
      _nxt_ptrslot = reinterpret_cast<memptr_t *> (last - 1);
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::compact_memslots (void)
  {
    memptr_t *p = _base;
    bigslot_t *m = _memslots->first;
    bigslot_t *n = NULL;

    memslot_list_t *nl = New memslot_list_t ();

    sanity_check();

    if (debug_warnings)
      warn << "+ compact memslots!\n";
    
    while (m) {
      m->check ();
      n = _memslots->next (m);
      _memslots->remove (m);
      bigslot_t *ns = reinterpret_cast<bigslot_t *> (p);
      if (m->v_data () > p) {
	ns->copy_reinit (m);
	ns->reseat ();
	p += ns->size ();
      }
      nl->insert_tail (ns);
      m = n;
    }

    delete (_memslots);
    _memslots = nl;

    sanity_check();

    _nxt_memslot = p;

    if (debug_warnings)
      warn << "- compact memslots!\n";
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::gc (void)
  {
    collect_ptrslots ();
    compact_memslots ();
    mark_deallocated (_nxt_memslot, _nxt_ptrslot - _nxt_memslot);
    _unclaimed_space = 0;
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
    int ret;
    if (mp < _base) ret = 1;
    else if (mp >= _base + _sz) ret = -1;
    else ret = 0;
    return ret;
  }

  //-----------------------------------------------------------------------

  static void
  dump_list (memslot_list_t *ml)
  {
    bigslot_t *p;
    warn ("List dump %p: ", ml);
    for (p = ml->first; p; p = ml->next (p)) {
      warn ("%p -> ", p);
    }
    warn ("NULL\n");
  }

  //-----------------------------------------------------------------------

  void
  bigobj_arena_t::remove (bigslot_t *s)
  { 
    if (debug_warnings >= 2) {
      dump_list (_memslots);
    }

    if (debug_warnings) {
      warn ("RM %p %p\n", s, s->_data);
    }

    mgr_t::get ()->sanity_check ();
    _memslots->remove (s); 

    if (debug_warnings >= 2)
      dump_list (_memslots);

    _unclaimed_space += s->size ();
    mgr_t::get ()->sanity_check ();
  }
  
  //=======================================================================
  
  static void *
  cgc_mmap (size_t sz)
  {
    void *v = mmap (NULL, sz, PROT_READ | PROT_WRITE, 
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    mark_unitialized (v, sz);

    if (!v) 
      panic ("mmap failed: %m\n");
    return v;
  }

  //-----------------------------------------------------------------------

  mmap_bigobj_arena_t::mmap_bigobj_arena_t (size_t sz)
  {

    sz = align(sz, get_pagesz ());
    void *v = cgc_mmap (sz);
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

  mmap_smallobj_arena_t::mmap_smallobj_arena_t (size_t sz, size_t l, size_t h,
						std_mgr_t *m, int i)
    : smallobj_arena_t (static_cast<memptr_t *> (cgc_mmap (sz)),
			sz, l, h, m, i) {}
  
  //=======================================================================

#ifdef CGC_DEBUG
  bool debug_mem = true;
#else /* CGC_DEBUG */
  bool debug_mem;
#endif /* CGC_DEBUG */

  int debug_warnings;

  void mark_deallocated (void *p, size_t sz)
  {
    if (debug_mem) {
      if (debug_warnings)
	warn ("mark deallocated: %p to %p\n", p, (char *)p + sz);
      memset (p, 0xdf, sz);
    }
  }

  void mark_unitialized (void *p, size_t sz)
  {
    if (debug_mem)
      memset (p, 0xca, sz);
  }

 //=======================================================================

  void
  bigptr_t::deallocate ()
  {
    check ();
    assert (_count == 0);
    _ms->check ();
    arena_t *a = mgr_t::get()->lookup (v_data ());
    assert (a);
    bigobj_arena_t *boa = a->to_boa();
    assert (boa);
    boa->check ();
    _ms->deallocate (boa);
    deallocate (boa);
  }

  //-----------------------------------------------------------------------

  void
  bigptr_t::deallocate (bigobj_arena_t *a)
  {
    check ();
    a->mark_free (this);
    _count = -1;
  }


  //=======================================================================

  size_t smallobj_sizer_t::_sizes[] =  { 4, 8, 12, 16,
					 24, 32, 40, 48, 56, 64,
					 80, 96, 112, 128, 
					 160, 192, 224, 256,
					 320, 384, 448, 512,
					 640, 768, 896, 1024 };
  
  //-----------------------------------------------------------------------

  smallobj_sizer_t::smallobj_sizer_t ()
    : _n_sizes (sizeof (_sizes) / sizeof (_sizes[0])) {}

  //-----------------------------------------------------------------------

  size_t
  smallobj_sizer_t::ind2size (int sz) const
  {
    if (sz <= 0) return 0;
    assert (sz < int (_n_sizes));
    return _sizes[sz];
  }

  //-----------------------------------------------------------------------

  size_t
  smallobj_sizer_t::find (size_t sz, int *ip) const
  {
    // Binary search the sizes vector (above)
    int lim = _n_sizes;

    int l, m, h;
    h = lim - 1;
    l = 0;
    while ( l <= h ) {
      m = (l + h)/2;
      if (_sizes[m] > sz) { h = m - 1; }
      else if (_sizes[m] < sz) { l = m + 1; }
      else { break; }
    }

    if (l < lim && _sizes[l] < sz) l++;

    size_t ret = 0;
    if (l < lim) ret = _sizes[l];
    else         l = -1;

    if (ip)
      *ip = l;

    return ret;
  }

  //=======================================================================


  //=======================================================================

  smallobj_arena_t::smallobj_arena_t (memptr_t *b, size_t s, size_t l, 
				      size_t h, std_mgr_t *m, int i)
    : arena_t (b, s),
      _top (b + s),
      _nxt (b),
      _min (l), 
      _max (h),
      _vacancy (true),
      _mgr (m),
      _soa_index (i)
  { 
    debug_init (); 
  }

  //-----------------------------------------------------------------------

  void
  smallobj_arena_t::mark_free (smallptr_t *p)
  {
    smallptr_t *base = reinterpret_cast<smallptr_t *> (_base);
    smallptr_t *top = reinterpret_cast<smallptr_t *> (_top);
    assert (p >= base);
    assert (p < top);
    _freemap.dealloc (p - base);
    if (!_vacancy) {
      _mgr->became_vacant (this, _soa_index);
      _vacancy = true;
    }
  }


  //-----------------------------------------------------------------------

  redirector_t
  smallobj_arena_t::aalloc (size_t sz)
  {
    redirector_t ret;
    assert (sz >= _min);
    assert (sz <= _max);
    memptr_t *mp = NULL;
    int pos = _freemap.alloc ();
    if (pos >= 0) {
      mp = _base + pos * _max;
    } else if (_nxt + _max  <= _top) {
      mp = _nxt;
      _nxt += _max;
    }
    assert (mp >= _base);
    assert (mp < _top);
    if (mp) {
      ret.init (reinterpret_cast<smallptr_t *> (mp));
    } else {
      _vacancy = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void 
  smallobj_arena_t::report () const
  {
    size_t nf = _freemap.nfree ();
    size_t nl = 0;
    if (_nxt < _top)
      nl = (_top - _nxt) / _max;

    warn (" smallobj_arena(%p -> %p): %zd-sized objs; %zd in freelist; "
	  "%zd unallocated\n",
	  _base, _top, _max, nf, nl); 
  }

  //=======================================================================
  
#define RDFN(nm, r, ...)	      \
  assert (_big || _small);	      \
  if (_big) r _big->nm (__VA_ARGS__); \
  else r _small->nm (__VA_ARGS__); 

  int32_t redirector_t::count () const { RDFN(count, return); }
  void redirector_t::set_count (int32_t i) { RDFN(set_count,,i); }
  size_t redirector_t::size () const { RDFN(size,return);  }
  memptr_t *redirector_t::v_data () { RDFN(v_data,return); }
  const memptr_t *redirector_t::v_data () const  { RDFN(v_data,return); }
  void redirector_t::deallocate () { RDFN(deallocate,); }

#undef RDFN

  //-----------------------------------------------------------------------

  memptr_t *
  redirector_t::obj ()
  {
    assert (count() > 0);
    return (v_data ()); 
  }

  //-----------------------------------------------------------------------
  
  const memptr_t *
  redirector_t::obj () const 
  { 
    assert (count() > 0);
    return (v_data ()); 
  }

  //-----------------------------------------------------------------------

  const memptr_t *
  redirector_t::lim () const
  {
    assert (count() > 0);
    return (v_data() + size());
  }
    
  //-----------------------------------------------------------------------

  void
  redirector_t::rc_inc ()
  {
    int32_t c = count ();
    assert (c >= 0);
    set_count (c + 1);
  }

  //-----------------------------------------------------------------------

  bool 
  redirector_t::rc_dec () 
  {
    bool ret;
    
    int c = count ();
    assert (c > 0);
    if (--c == 0) {
      ret = false;
    } else {
      ret = true;
    }
    set_count (c);
    return ret;
  }
  
  //=======================================================================

  smallobj_arena_t *
  smallptr_t::lookup_arena () const
  {
    arena_t *a = mgr_t::get()->lookup (v_data ());
    assert (a);
    smallobj_arena_t *soa = a->to_soa ();
    assert (soa);
    soa->check ();
    return soa;
  }

  //-----------------------------------------------------------------------

  size_t
  smallptr_t::size () const
  {
    return lookup_arena ()->slotsize ();
  }

  //-----------------------------------------------------------------------

  void
  smallptr_t::deallocate ()
  {
    deallocate (lookup_arena ());
  }

  //-----------------------------------------------------------------------

  void
  smallptr_t::deallocate (smallobj_arena_t *a) 
  {
    check ();
    mark_free ();
    a->mark_free (this);
  }

  //=======================================================================

  redirector_t
  soa_cluster_t::aalloc (size_t sz)
  {
    smallobj_arena_t *a, *n;
    redirector_t ret;
    for (a = _vacancy.first; !ret && a; a = n) {
      assert (a->_vacancy_list_id == true);
      n = _vacancy.next (a);
      if (!(ret = a->aalloc (sz))) {
	_vacancy.remove (a);
	_no_vacancy.insert_tail (a);
	a->_vacancy_list_id = false;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  soa_cluster_t::add (smallobj_arena_t *a)
  {
    _vacancy.insert_tail (a);
    a->_vacancy_list_id = true;
  }

  //-----------------------------------------------------------------------

  void
  soa_cluster_t::became_vacant (smallobj_arena_t *a)
  {
    assert (a->_vacancy_list_id == false);
    _no_vacancy.remove (a);
    _vacancy.insert_tail (a);
    a->_vacancy_list_id = true;
  }

  //=======================================================================

};
