
// -*- c++ -*-

#include "refcnt.h"
#include "callback.h"
#include "list.h"
#include "vec.h"
#include "itree.h"

#define CGC_DEBUG 1


namespace cgc {

  typedef u_int8_t memptr_t;

  class bigptr_t;

  extern bool debug_mem;
  extern int debug_warnings;
  void mark_deallocated (void *, size_t);
  void mark_unitialized (void *, size_t);

  //=======================================================================

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

    tailq_entry<bigslot_t> _next;
    size_t _sz;
    bigptr_t *_ptrslot;

    memptr_t _data[0];

    static size_t size (size_t s);
    size_t size () const { check(); return size (_sz); }

    memptr_t *v_data () { check(); return _data; }
    const memptr_t *v_data () const { check(); return _data; }
    void reseat ();

    void copy_reinit (const bigslot_t*ms);
    void slotfree ();

  };

  //=======================================================================

  typedef tailq<bigslot_t, &bigslot_t::_next> memslot_list_t;

  //=======================================================================

  class redirect_ptr_t {
  public:
    
    redirect_ptr_t () {}
    virtual ~redirect_ptr_t () {}
    
    virtual int count() const = 0;
    virtual void set_count (int i) = 0;
    virtual size_t size () const = 0;
    virtual memptr_t *v_data () = 0;
    virtual const memptr_t *v_data () const = 0;
    virtual void slotfree () = 0;

    memptr_t *obj () 
    { 
      assert (count() > 0);
      return (v_data ()); 
    }

    const memptr_t *obj () const 
    { 
      assert (count() > 0);
      return (v_data ()); 
    }

    const memptr_t *lim () const
    {
      assert (count() > 0);
      return (v_data() + size());
    }

    void rc_inc () { set_count (count () + 1); }

    bool rc_dec () {
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
  };

  //=======================================================================

  class bigptr_t : public redirect_ptr_t {
  public:
    bigptr_t (bigslot_t *m) : _ms (m), _count (0) { debug_init (); }
    void set_mem_slot (bigslot_t *ms) { _ms = ms; }
    bigslot_t *memslot () const { return _ms; }
    void init (bigslot_t *m, int c)  { _ms = m; _count = c; }
    void mark_free () { _count = -1; }
    int count () const { return _count; }
    void set_count (int i) { _count = i; }
    size_t size () const { return _ms->size (); }
    memptr_t *v_data () { return _ms->v_data (); }
    const memptr_t *v_data () const { return _ms->v_data (); }
    void slotfree ();
  protected:
    bigslot_t *_ms;
    int _count;

  protected:

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

  };

  //=======================================================================

  template<class T> class alloc;
  template<class T> class alloc2;

  //=======================================================================

  template<class T>
  class ptr {
  public:
    ptr (const ptr<T> &p) : _redir_ptr (p._redir_ptr) { rc_inc (); }
    ptr () : _redir_ptr (NULL) {}
    virtual ~ptr () { rc_dec(); }

    const T *volatile_ptr () const { return obj (); }
    T *volatile_ptr () { return obj (); }

    void rc_inc ()
    {
      if (_redir_ptr) { _redir_ptr->rc_inc (); }
    }

    void rc_dec ()
    {
      if (_redir_ptr && !_redir_ptr->rc_dec ()) {
	obj()->~T();
	_redir_ptr->slotfree ();
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
    T &operator* () const { return nonnull_volatile_ptr (); }
    operator bool() const { return volatile_ptr () != NULL; }

    bool operator== (const ptr<T> &p) const { return base_eq (p); }
    bool operator!= (const ptr<T> &p) const { return !base_eq (p); }

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
      _redir_ptr = NULL;
      v_clear ();
      return (*this);
    }

    friend class alloc<T>;
    friend class alloc2<T>;
  protected:
    explicit ptr (redirect_ptr_t *p) : _redir_ptr (p) { rc_inc (); }

    bool base_eq (const ptr<T> &p) { return _redir_ptr == p._redir_ptr; }

    virtual void v_clear () {}

    T *obj ()
    {
      if (_redir_ptr) return reinterpret_cast<T *> (_redir_ptr->v_data ());
      else return NULL;
    }

    const T *obj () const
    {
      if (_redir_ptr) 
	return reinterpret_cast<const T *> (_redir_ptr->v_data ());
      else return NULL;
    }

    const T *lim () const
    {
      if (_redir_ptr) return reinterpret_cast<const T*> (_redir_ptr->lim ());
      else return NULL;
    }

    redirect_ptr_t *_redir_ptr;

  private:
    explicit ptr (int i); // do not call list
  };

  //=======================================================================

  template<class T>
  class aptr : public ptr<T> {
  public:
    aptr () : ptr<T> (), _offset (0) {}
    aptr (size_t s) : ptr<T> (), _offset (s) {}
    
    bool operator== (const aptr<T> &p) const
    { return base_eq (p) && _offset = p._offset;  }

    bool operator== (const aptr<T> &p) const
    { return ! ( (*this) == p); }
    
    aptr<T> &operator= (aptr<T> &p)
    {
      ptr<T>::rc_dec ();
      ptr<T>::_redir_ptr = p._redir_ptr;
      _offset = p._offset;
      ptr<T>::rc_inc ();
      return (*this);
    }

    aptr<T> operator+ (size_t s) const
    {
      aptr<T> ret (ptr<T>::_redir_ptr, _offset + s);
      assert (ret.inbounds ());
      return ret;
    }

    aptr<T> operator- (size_t s) const
    {
      assert (s <= _offset);
      aptr<T> ret (ptr<T>::_redir_ptr, _offset - s);
      return ret;
    }

    aptr<T> operator++ (size_t s) { return (*this) += 1; }
    aptr<T> operator-- (size_t s) { return (*this) -= 1; }

    aptr<T> &operator+= (size_t s) 
    {
      _offset += s;
      assert (inbounds ());
      return (*this);
    }

    aptr<T> &operator-= (size_t s) 
    {
      assert (s <= _offset);
      _offset -= s;
      return (*this);
    }

    bool operator< (const aptr<T> &p) const 
    { 
      assert (base_eq (p));
      return (_offset < p._offset); 
    } 

    bool operator<= (const aptr<T> &p) const 
    { 
      assert (base_eq (p));
      return (_offset <= p._offset); 
    }
 
    bool operator>= (const aptr<T> &p) const 
    { 
      assert (base_eq (p));
      return (_offset >= p._offset); 
    } 

    bool operator> (const aptr<T> &p) const 
    { 
      assert (base_eq (p));
      return (_offset > p._offset); 
    } 

    bool inbounds ()
    {
      return (ptr<T>::obj() + _offset <= ptr<T>::lim());
    }

  protected:
    aptr (redirect_ptr_t *p, size_t s) : ptr<T> (p), _offset (s) {}

  private:

    T *obj ()
    {
      assert (inbounds ());
      return (ptr<T>::obj() + _offset);
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

    void push_back (const T &t)
    {
      reserve ();
      assert (_nxt < _size);
      _base[_nxt++] = t;
    }

    size_t size () const { return _size; }
    
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

  class bigobj_arena_t;

  class arena_t {
  public:
    arena_t (memptr_t *base, size_t sz) : _base (base), _sz (sz) {}
    virtual ~arena_t () {}

    virtual void remove (bigslot_t *s) = 0;

    virtual redirect_ptr_t *aalloc (size_t sz) = 0;
    virtual bool gc_make_room (size_t sz) { return false; }

    int cmp (const memptr_t *m) const;

    // make a tree of all active arenas, so that we can take an
    // object and figure out which arena it lives in.
    itree_entry<arena_t> _tlnk;
    memptr_t *_base;
  protected:
    size_t _sz;
  };


  //=======================================================================

  class bigobj_arena_t : public arena_t {
  public:
    bigobj_arena_t (memptr_t *base, size_t sz) 
      : arena_t (base, sz), 
	_memslots (New memslot_list_t ()),
	_unclaimed_space (0) { init(); }
    bigobj_arena_t () 
      : arena_t (NULL, 0), 
	_top (NULL), 
	_nxt_ptrslot (NULL), 
	_nxt_memslot (NULL),
	_memslots (New memslot_list_t ()),
	_unclaimed_space (0) {}

    void init ();

    virtual ~bigobj_arena_t () {}

    redirect_ptr_t *aalloc (size_t sz);
    void gc (void);
    virtual bool gc_make_room (size_t sz);

    tailq_entry<bigobj_arena_t> _qlnk;

    void remove (bigslot_t *m);
    bool can_fit (size_t sz) const;
    size_t free_space () const;
    
    void sanity_check () const;

  protected:
    bigptr_t *get_free_ptrslot (void);
    void collect_ptrslots (void);
    void compact_memslots (void);

    memptr_t *_top;
    memptr_t *_nxt_ptrslot;
    memptr_t *_nxt_memslot;

    memslot_list_t *_memslots;
    simple_stack_t<bigptr_t *> _free_ptrslots;
    size_t _unclaimed_space;
  };

  //=======================================================================

  class mmap_bigobj_arena_t : public bigobj_arena_t {
  public:
    mmap_bigobj_arena_t (size_t sz);
    ~mmap_bigobj_arena_t ();
    
    static size_t pagesz;
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

  class mgr_t {
  public:
    mgr_t () {}
    virtual ~mgr_t () {}

    virtual arena_t *pick (size_t sz) = 0;

    template<class T> arena_t *
    lookup (ptr<T> p) { return lookup (p->volatile_ptr ()); }

    redirect_ptr_t *aalloc (size_t sz) ;

    arena_t *lookup (const memptr_t *p);
    virtual void sanity_check (void) const {}

    void insert (arena_t *a);
    void remove (arena_t *a);
    static mgr_t *get ();
    static void set (mgr_t *m);

  private:
    itree<memptr_t *, arena_t, &arena_t::_base, &arena_t::_tlnk> _tree;

  };
  
  //=======================================================================

  template<class T>
  class alloc2 {
  public:
    template<class A> explicit alloc2 (const A &a)
    {
      redirect_ptr_t *r = mgr_t::get()->aalloc(sizeof(T));
      if (r) {
	(void) new (r->v_data ()) T (a);
	_p = ptr<T> (r);
      }
    }
    operator ptr<T>& () { return _p; }
    operator const ptr<T>& () const { return _p; }
  private:
    ptr<T> _p;
  };

  //=======================================================================

  template<class T>
  class alloc {
  public:
    VA_TEMPLATE(explicit alloc ,					\
		{ redirect_ptr_t *r = mgr_t::get()->aalloc(sizeof(T));	\
		  if (r) {						\
		  (void) new (r->v_data ()) T ,				\
		    ; _p = ptr<T> (r); } } )
    operator ptr<T>&() { return _p; }
    operator const ptr<T> &() const { return _p; }
  private:
    ptr<T> _p;
  };

  //=======================================================================

  struct std_cfg_t {
    std_cfg_t ()
      : _n_b_arenae (0x10),
	_size_b_arenae (0x100) {}

    size_t _n_b_arenae;
    size_t _size_b_arenae;
  };

  //=======================================================================


  class std_mgr_t : public mgr_t {
  public:
    std_mgr_t (const std_cfg_t &cfg);
    arena_t *pick (size_t sz);

    typedef tailq<bigobj_arena_t, &bigobj_arena_t::_qlnk> boa_list_t;

    virtual bigobj_arena_t *gc_make_room_big (size_t sz);
    void sanity_check (void) const;

  private:
    std_cfg_t _cfg;
    boa_list_t _bigs;
    bigobj_arena_t *_next_big;
  };


  //=======================================================================

};
