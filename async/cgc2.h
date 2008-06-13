
// -*- c++ -*-

#include "refcnt.h"
#include "callback.h"
#include "list.h"
#include "vec.h"
#include "itree.h"
#include "freemap.h"

#define CGC_DEBUG 1

namespace cgc {

  typedef u_int8_t memptr_t;

  template<class T, class G> class bigptr_t;
  template<class T, class G> class smallptr_t;

  extern bool debug_mem;
  extern int debug_warnings;
  void mark_deallocated (void *, size_t);
  void mark_unitialized (void *, size_t);

  template class bigobj_arena_t<class T, class G>;

  //=======================================================================

  template<class T>
  class wkref {
  public:
    wkref (T &obj) : _obj (&obj), _alive (obj->alive ()) {}
    wkref () : _obj (NULL) {}
    operator bool() const { return (*_alive && _obj); }
    const T *operator-> () const { return obj(); }
    T *operator->() { return obj (); }
    T &operator* () const { return *obj(); }
  private:
    T *obj () { return (*_alive ? _obj : NULL); }
    const T *obj () const { return (*_alive ? _obj : NULL); }
    ptr<bool> _alive;
    T *_obj;
  };


  //=======================================================================

  class lru_obj_t {
  public:
    virtual ~lru_obj_t () {}
    virtual void touch () = 0;
    virtual void mark () = 0;
  };

  //=======================================================================

  class lru_mgr_t {
  public:
    virtual ~lru_mgr_t () {}
    virtual void start_mark_phase () = 0;
    virtual void end_mark_phase () = 0;
  };

  //=======================================================================

  namespace nil {

    template<class T>
    struct ptr_t {
      operator bool() const { return false; }
      T *operator-> () { return NULL; }
      const T *operator-> () const { return NULL; }
    };
    
    struct gc_obj_t {
      void mark () {}
      void touch () {}
    };

    typedef ptr_t<gc_obj_t> gc_ptr_t;
    
    struct t {};
  };

  //=======================================================================

  template<class T, class G = nil::gc_ptr_t>
  class bigslot_t {
  public:
    bigslot_t (size_t sz, bigptr_t *p);

#ifdef CGC_DEBUG
    u_int32_t _magic;
    enum { magic = 0xfbeefbee };

    void debug_init () 
    {
      _magic = magic;
    }

    void check () const
    {
      assert (_magic == magic);
    }

#else /* CGC_DEBUG */

    void debug_init () {}
    void check () const {}
    
#endif /*CGC_DEBUG */

    tailq_entry<typename bigslot_t<T,G> > _next;
    size_t _sz;
    typename bigptr_t<T,G> *_ptrslot;

    // pointer to referer obj for garbage collection
    // Use template tricks so it takes up 0 bytes if not present.
    G _gcp;

    void set_lru_ptr (const G &r) { _gc_obj_ptr = r; }

    T _data[0];

    static size_t size (size_t s);
    size_t size () const { check(); return size (_sz); }

    T *data () { check(); return _data; }
    const T *data () const { check(); return _data; }
    void reseat ();
    void mark () { if (_gcp) _gcp->mark (); }

    void copy_reinit (const bigslot_t*ms);
    void deallocate (typename bigobj_arena_t<T,G> *a);
    void touch ();

  };

  //=======================================================================

  template<class T, class G>
  struct types {
    typedef tailq<bigslot_t<T,G>, &bigslot_t<T,G>::_next> memslot_list_t;
  };

  //=======================================================================
    
  template<class T, class G = nil::gc_ptr_t>
  class redirector_t {
  public:
    redirector_t (typename bigptr_t<T,G> *b) : _sel (BIG), _big (b) {}
    redirector_t (typename smallptr_t<T,G> *s) : _sel (SMALL), _small (s) {}
    redirector_t () : _sel (NONE), _none (NULL) {}

    void init (typename bigptr_t<T,G> *b) { _big = b; _sel = BIG; }
    void init (typename smallptr_t<T,G> *s) { _small = s; _sel = SMALL; }

    int32_t count() const;
    void set_count (int32_t i);
    size_t size () const;
    T *data ();
    const T *data () const;
    void deallocate ();

    // LRU opts
    void set_lru_ptr (const G &p);
    void touch ();

    T *obj ();
    const T *obj () const;
    const T *lim () const;
    void rc_inc ();
    bool rc_dec ();
    operator bool() const { return is_non_null (); }
    void clear () { _sel = NONE; _none = NULL; }

    typedef enum { NONE, BIG, SMALL, STD } selector_t;

  private:
    bool is_non_null () const { return (_sel != NONE && _none); }
    typename bigptr_t<T,G> *big () { return (_sel == BIG ? _big : NULL); }
    selector_t _sel;
    union {
      void *_none;
      typename bigptr_t<T,G> *_big;
      typename smallptr_t<T,G> *_small;
    };
  };

  //=======================================================================

  template<class T, class G = nil::gc_ptr_t>
  class bigptr_t { // implements the redirector_t interface
  public:
    typedef typename bigslot_t<T,G> slot_t;

    bigptr_t (slot_t *m) : _ms (m), _count (0) { debug_init (); }
    void set_mem_slot (slot_t *ms) { _ms = ms; }
    bigslot_t *memslot () const { return _ms; }
    void init (bigslot_t *m, int32_t c)  { _ms = m; _count = c; }
    void mark_free () { _count = -1; }
    int32_t count () const { return _count; }
    void set_count (int32_t i) { _count = i; }
    size_t size () const { return _ms->size (); }
    memptr_t *v_data () { return _ms->v_data (); }
    const memptr_t *v_data () const { return _ms->v_data (); }
    void deallocate ();
    void set_lru_ptr (const G &o) { _ms->set_lru_ptr (o); }
    void touch () { _ms->touch (); }

    friend class typename bigobj_arena_t<T,G>;
  protected:
    void deallocate (typename bigobj_arena_t<T,G> *a);
    
#ifdef CGC_DEBUG
    u_int32_t _magic;
    enum { magic = 0xefbeefbe };
    void debug_init () 
    {
      _magic = magic;
    }
    void check () const
    {
      assert (_magic == magic);
    }
#else /* CGC_DEBUG */
    void debug_init () {}
    void check () const {}
#endif /*CGC_DEBUG */

    slot_t *_ms;
    int32_t _count;
    
  };

  //=======================================================================

  template<class T> class alloc;
  template<class T> class vecalloc;


  //=======================================================================

  template<class T, class V> 
  class caster_t {
  public:
  };

  template<class T>
  class caster_t<T,T> {
  public:
    static T *cast (T *in) { return in; }
    static const T *cast (const T *in) { return in; }
  };

  template<class T>
  class caster_t<T, memptr_t> {
  public:
    static T *cast (memptr_t *in) { return reinterpret_cast<T *> (in); }
    static const T *cast (const memptr_t *in) 
    { return reinterpret_cast<const T *> (in); }
  };

  //=======================================================================

  template<class T, class V, class G = nil::gc_ptr_t >
  class ptr {
  public:
    ptr (const ptr<T> &p) : _redir_ptr (p._redir_ptr) { rc_inc (); }
    ptr () {}
    virtual ~ptr () { rc_dec(); }

    const T *volatile_ptr () const { return obj (); }
    T *volatile_ptr () { return obj (); }

    void rc_inc () { if (_redir_ptr) { _redir_ptr.rc_inc (); } }

    void rc_dec ()
    {
      if (_redir_ptr && !_redir_ptr.rc_dec ()) {
	obj()->~T();
	_redir_ptr.deallocate ();
	_redir_ptr.clear ();
      }
    }

    const T *nonnull_volatile_ptr () const 
    {
      const T *ret = obj();
      assert (ret);
      return ret;
    }

    T *nonnull_volatile_ptr () 
    {
      T *ret = volatile_ptr ();
      assert (ret);
      return ret;
    }

    const T *operator-> () const { return nonnull_volatile_ptr (); }
    T *operator-> () { return nonnull_volatile_ptr (); }
    T &operator* () { return *nonnull_volatile_ptr (); }
    const T &operator* () const { return *nonnull_volatile_ptr (); }
    operator bool() const { return volatile_ptr () != NULL; }

    bool operator== (const ptr<T> &p) const { return base_eq (p); }
    bool operator!= (const ptr<T> &p) const { return !base_eq (p); }

    void set_lru_ptr (const G &p) { _redir_ptr.set_lru_ptr (p); }
    void touch () { _redir_ptr.touch (); }

    ptr<T> &operator= (const ptr<T> &p)
    {
      rc_dec ();
      _redir_ptr = p._redir_ptr;
      v_clear ();
      rc_inc ();
      return (*this);
    }

    ptr<T> &operator= (int i)
    {
      assert (i == 0);
      rc_dec ();
      _redir_ptr.clear ();
      v_clear ();
      return (*this);
    }

    friend class alloc<T>;
    friend class vecalloc<T>;
  protected:
    explicit ptr (typename const redirector_t<V,G> &p) 
      : _redir_ptr (p) { rc_inc (); }

    bool base_eq (const ptr<T> &p) { return _redir_ptr == p._redir_ptr; }

    virtual void v_clear () {}

    T *obj ()
    {
      if (_redir_ptr) return caster_t<T,V>::cast (_redir_ptr.data ());
      else return NULL;
    }

    const T *obj () const
    {
      if (_redir_ptr) return caster_t<T,V>::cast (_redir_ptr.data ());
      else return NULL;
    }

    const T *lim () const
    {
      if (_redir_ptr) return caster_t<T,V>::cast (_redir_ptr.lim ());
      else return NULL;
    }

    typename redirector_t<V,G> _redir_ptr;

  private:
    explicit ptr (int i); // do not call list
  };

  //=======================================================================

  template<class T, class V, class G = nil:gc_ptr_t>
  class aptr : public ptr<T,V,G> {
  public:
    aptr () : ptr<T,V,G> (), _offset (0) {}
    aptr (size_t s) : ptr<T,V,G> (), _offset (s) {}
    
    bool operator== (const aptr<T,V,G> &p) const
    { return base_eq (p) && _offset = p._offset;  }

    bool operator== (const aptr<T,V,G> &p) const
    { return ! ( (*this) == p); }
    
    aptr<T,V,G> &operator= (aptr<T,V,G> &p)
    {
      ptr<T,V,G>::rc_dec ();
      ptr<T,V,G>::_redir_ptr = p._redir_ptr;
      _offset = p._offset;
      ptr<T,V,G>::rc_inc ();
      return (*this);
    }

    aptr<T,V,G> operator+ (size_t s) const
    {
      aptr<T,V,G> ret (ptr<T,V,G>::_redir_ptr, _offset + s);
      assert (ret.inbounds ());
      return ret;
    }

    aptr<T,V,G> operator- (size_t s) const
    {
      assert (s <= _offset);
      aptr<T,V,G> ret (ptr<T,V,G>::_redir_ptr, _offset - s);
      return ret;
    }

    aptr<T,V,G> operator++ (size_t s) { return (*this) += 1; }
    aptr<T,V,G> operator-- (size_t s) { return (*this) -= 1; }

    aptr<T,V,G> &operator+= (size_t s) 
    {
      _offset += s;
      assert (inbounds ());
      return (*this);
    }

    aptr<T,V,G> &operator-= (size_t s) 
    {
      assert (s <= _offset);
      _offset -= s;
      return (*this);
    }

    bool operator< (const aptr<T,V,G> &p) const 
    { 
      assert (base_eq (p));
      return (_offset < p._offset); 
    } 

    bool operator<= (const aptr<T,V,G> &p) const 
    { 
      assert (base_eq (p));
      return (_offset <= p._offset); 
    }
 
    bool operator>= (const aptr<T,V,G> &p) const 
    { 
      assert (base_eq (p));
      return (_offset >= p._offset); 
    } 

    bool operator> (const aptr<T,V,G> &p) const 
    { 
      assert (base_eq (p));
      return (_offset > p._offset); 
    } 

    bool inbounds ()
    {
      return (ptr<T,V,G>::obj() + _offset <= ptr<T,V,G>::lim());
    }

  protected:
    aptr (const redirector_t &p, size_t s) : ptr<T,V,G> (p), _offset (s) {}

  private:

    T *obj ()
    {
      assert (inbounds ());
      return (ptr<T,V,G>::obj() + _offset);
    }

    void v_clear () { _offset = 0; }

    size_t _offset;
  };

  //=======================================================================

  template<class T>
  class simple_stack_t {
  public:

    enum { defsize = 0x10 } ;

    simple_stack_t () 
      : _base (New T [defsize]),
	_nxt (0),
	_size (defsize) {}

    ~simple_stack_t () { delete [] _base; }

    void clear () { _nxt = 0; }

    void push_back (const T &t)
    {
      reserve ();
      assert (_nxt < _size);
      _base[_nxt++] = t;
    }

    T pop_back () 
    {
      assert (_nxt > 0);
      T ret = _base[--_nxt];
      return ret;
    }

    T back () const
    {
      assert (_nxt > 0);
      return _base[_nxt-1];
    }

    const T &operator[] (size_t s) const
    {
      assert (s < _nxt);
      return _base[s];
    }

    void reserve ()
    {
      if (_nxt == _size) {
	size_t newsz = _size * 2;
	T *nb = New T[newsz];
	for (size_t i = 0; i < _nxt ; i++) {
	  nb[i] = _base[i];
	}
	delete [] _base;
	_base = nb;
	_size = newsz;
      }
    }

    size_t n_elem () const { return _nxt; }
    
  private:
    T *_base;
    size_t _nxt, _size;
  };

  //=======================================================================

  class smallobj_arena_t;

  template<class T, class G = nil::gc_ptr_t>
  class arena_t {
  public:
    arena_t (memptr_t *base, size_t sz) : _base (base), _sz (sz) {}
    virtual ~arena_t () {}

    virtual redirector_t aalloc (size_t sz) = 0;
    virtual bool gc_make_room (size_t sz) { return false; }
    virtual void report (void) const {}
    virtual void gc (lru_mgr_t *m) = 0;
    virtual bigobj_arena_t<T,G> *to_boa () { return NULL; }
    virtual smallobj_arena_t<T,G> *to_soa () { return NULL; }

    int cmp (const memptr_t *m) const;

    // make a tree of all active arenas, so that we can take an
    // object and figure out which arena it lives in.
    itree_entry<arena_t<T,G> > _tlnk;
    memptr_t *_base;
  protected:
    size_t _sz;
  };


  //=======================================================================

  template<class T = memptr_t, class G = nil::gc_ptr_t>
  class bigobj_arena_t : public arena_t {
  public:
    bigobj_arena_t (memptr_t *base, size_t sz) 
      : arena_t (base, sz), 
	_memslots (New types<T,G>::memslot_list_t ()),
	_unclaimed_space (0) { debug_init(); init(); }
    bigobj_arena_t () 
      : arena_t (NULL, 0), 
	_top (NULL), 
	_nxt_ptrslot (NULL), 
	_nxt_memslot (NULL),
	_memslots (New types<T,G>::memslot_list_t ()),
	_unclaimed_space (0) { debug_init(); }

    void init ();

    virtual ~bigobj_arena_t () {}

    typename redirector_t<T,G> aalloc (size_t sz);
    void gc (lru_mgr_t *m);
    virtual bool gc_make_room (size_t sz);

    tailq_entry<bigobj_arena_t<T,G> > _qlnk;

    bool can_fit (size_t sz) const;
    size_t free_space () const;
    
    void sanity_check () const;
    virtual void report (void) const;

    void debug_init () { _magic = magic; }
    void check() { assert (magic == _magic); }
    void mark_free (typename bigptr_t<T,G>  *p);
    void remove (typename bigslot_t<T,G> *p);
    
    bigobj_arena_t *to_boa () { return this; }

  protected:
    typename bigptr_t<T,G> *get_free_ptrslot (void);
    void collect_ptrslots (void);
    void compact_memslots (void);
    void lru_accounting (lru_mgr_t *m);

    enum { magic = 0x4ee3beef };
    u_int32_t _magic;

    memptr_t *_top;
    memptr_t *_nxt_ptrslot;
    memptr_t *_nxt_memslot;

    types<T,G>::memslot_list_t *_memslots;
    simple_stack_t<bigptr_t<T,G> *> _free_ptrslots;
    size_t _unclaimed_space;
  };

  //=======================================================================

  class smallobj_sizer_t {
  public:
    smallobj_sizer_t ();
    size_t find (size_t sz, int *ip = NULL) const;
    size_t ind2size (int ind) const;

  private:
    static size_t _sizes[];
    size_t _n_sizes;
  };

  //=======================================================================

  class smallobj_arena_t;

  template<class T, class G = nil::gc_ptr_t>
  class smallptr_t { // implements redirector interface
  public:
    smallptr_t () : _count (0) { debug_init (); }
    void init () { _count = 0; }
    void mark_free () { _count = -1; }
    int32_t count () const { return _count; }
    void set_count (int32_t i) { _count = i; }
    size_t size () const;
    T *data () { return _data; }
    const T *data () const { return _data; }
    void deallocate ();

    friend class smallobj_arena_t<T,G>;
  protected:
    void deallocate (smallobj_arena_t<T,G> *a);
    smallobj_arena_t *lookup_arena () const;
    
#ifdef CGC_DEBUG
    u_int32_t _magic;
    enum { magic = 0x12beef43 };
    void debug_init () 
    {
      _magic = magic;
    }
    void check () const
    {
      assert (_magic == magic);
    }
#else /* CGC_DEBUG */
    void debug_init () {}
    void check () const {}
#endif /*CGC_DEBUG */

    int32_t _count;
    G _gc_obj;
    T _data[0];
  };

  //=======================================================================

  class std_mgr_t;

  template<class T, class G = nil::gc_ptr_t>
  class smallobj_arena_t : public arena_t {
  public:
    smallobj_arena_t (memptr_t *b, size_t sz, size_t l, size_t h,
		      std_mgr_t<T,G> *m, int i);

    redirector_t aalloc (size_t sz);
    void report (void) const;
    void gc (lru_mgr_t *) {}
    size_t slotsize () const { return _max; }

    smallobj_arena_t<T,G> *to_soa () { return this; }
    void check () { assert (_magic == magic); }
    void mark_free (smallptr_t *p);

    static size_t crud_size (size_t objsz);
    bool vacancy () const { return _vacancy; }

    tailq_entry<smallobj_arena_t<T,G> > _soa_lnk;
  protected:
    void debug_init () { _magic = magic; }

    enum { magic = 0xdead1121 };
    u_int32_t _magic;

    T *_top, *_nxt;
    freemap_t _freemap;
    size_t _min, _max;
    bool _vacancy;
    std_mgr_t<T,G> *_mgr;
    int _soa_index;
  public:
    bool _vacancy_list_id;
  };

  //=======================================================================

  template<class T, class G = nil::gc_ptr_t>
  class mmap_bigobj_arena_t : public bigobj_arena_t<T,G> {
  public:
    mmap_bigobj_arena_t (size_t sz);
    ~mmap_bigobj_arena_t ();
  };

  //=======================================================================

  template<class T>
  class allocator_t {
  public:
    allocator_t () {}
    virtual ~allocator_t () {}
    virtual ptr<T> alloc () = 0;
  };

  //=======================================================================

  template<class T, class G = nil::gc_ptr_t>
  class mgr_t {
  public:
    mgr_t () {}
    virtual ~mgr_t () {}

    template<class R> arena_t *
    lookup (ptr<R,T,G> p) { return lookup (p->volatile_ptr ()); }

    virtual redirector_t<T,G> aalloc (size_t sz) = 0;

    arena_t<T,G> *lookup (const memptr_t *p);
    virtual void sanity_check (void) const {}
    virtual void report (void) const {}

    void insert (arena_t<T,G> *a);
    void remove (arena_t<T,G> *a);
    static mgr_t<T,G> *get ();
    static void set (mgr_t<T,G> *m);
    virtual void gc (void) = 0;
    
  private:
    static mgr_t<T,G> *_mgr;

    itree<memptr_t *, arena_t<T,G>, 
	  &arena_t<T,G>::_base, &arena_t<T,G>::_tlnk> _tree;
  };
  
  //=======================================================================

#define COMMA ,

  template<class T, class V = memptr, class G = nil::gc_ptr_t>
  class alloc {
  public:
    VA_TEMPLATE(explicit alloc ,					\
		{ redirector_t<V,G> r =					\
		    mgr_t::get()->aalloc(sizeof(T));			\
		  if (r) {						\
		    (void) new (r.data ()) T ,				\
		      ; _p = ptr<T COMMA V COMMA G> (r); } } )
    operator ptr<T,V,G>&() { return _p; }
    operator const ptr<T,V,G> &() const { return _p; }
  private:
    ptr<T,V,G> _p;
  };

#undef COMMA

  //=======================================================================

  template<class T, class V = memptr, class G = nil::gc_ptr_t>
  class vecalloc {
  public:
    explicit vecalloc (size_t n) {
      redirector_t<V,G> r = mgr_t<V,G>::get()->aalloc(sizeof(T)*n);
      if (r) {
	(void) new (r.data ()) T [n];
	_p = ptr<T,V,G> (r); 
      } 
    } 
    operator ptr<T,V,G>&() { return _p; }
    operator const ptr<T,V,G> &() const { return _p; }
  private:
    ptr<T,V,G> _p;
  };

  //=======================================================================

  class referee_t {
  public:
    referee_t () : _alive (alloc<bool> (true)) {}
    ~referee_t () { *_alive = false; }
    ptr<bool> alive () { return _alive; }
  private:
    ptr<bool> _alive;
  };

  //=======================================================================

  struct std_cfg_t {
    std_cfg_t ()
      : _n_b_arenae (0x10),
	_size_b_arenae (0x100),
	_smallobj_lim (-1),
	_smallobj_max_overhead_pct (25) {}

    size_t _n_b_arenae;
    size_t _size_b_arenae;
    ssize_t _smallobj_lim;
    size_t _smallobj_max_overhead_pct;
    
  };

  //=======================================================================

  template<class T, class G = nil::gc_obj_t>
  class soa_cluster_t {
  public:
    soa_cluster_t (size_t s) : _size (s) {}
    typedef tailq<smallobj_arena_t<T,G>, 
		  &smallobj_arena_t<T,G>::_soa_lnk> soa_list_t;

    redirector_t<T,G> aalloc (size_t sz);
    size_t _size;

    void became_vacant (smallobj_arena_t<T,G> *a);
    void add (smallobj_arena_t<T,G> *a);
  private:
    soa_list_t _vacancy;
    soa_list_t _no_vacancy;
  };

  //=======================================================================

  template<class T, class G = nil::gc_obj_t>
  class std_mgr_t : public mgr_t {
  public:
    std_mgr_t (const std_cfg_t &cfg);

    typedef tailq<bigobj_arena_t<T,G>, &bigobj_arena_t<T,G>::_qlnk> boa_list_t;

    virtual bigobj_arena_t<T,G> *gc_make_room_big (size_t sz);
    virtual void sanity_check (void) const;
    virtual void report (void) const;
    virtual void gc (void);
    redirector_t<T,G> aalloc (size_t sz);

    void set_lru_mgr (lru_mgr_t<G> *m) { _lru_mgr = m; }

    friend class smallobj_arena_t<T,G>;
  protected:
    redirector_t<T,G> big_alloc (size_t sz);
    redirector_t<T,G> small_alloc (size_t sz);
    bigobj_arena_t<T,G> *big_pick (size_t sz);
    void became_vacant (smallobj_arena_t<T,G> *a, int i);
    smallobj_arena_t<T,G> *alloc_soa (size_t sz, int ind);

    std_cfg_t _cfg;
    boa_list_t _bigs;
    bigobj_arena_t<T,G> *_next_big;
    smallobj_sizer_t _sizer;

    vec<soa_cluster_t<T,G> *> _smalls;
    size_t _smallobj_lim;
    lru_mgr_t<G> *_lru_mgr;
  };


  //=======================================================================

  template<class T, class G = nil::gc_obj_t>
  class mmap_smallobj_arena_t : public smallobj_arena_t<T,G> {
  public:
    mmap_smallobj_arena_t (size_t sz, size_t l, size_t h,
			   std_mgr_t<T,G> *m, int i);
    ~mmap_smallobj_arena_t () {}
  };

  //=======================================================================

};

#include "cgc2-impl.h"
