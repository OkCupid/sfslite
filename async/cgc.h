
// -*- c++ -*-

#include "refcnt.h"
#include "callback.h"
#include "list.h"
#include "vec.h"
#include "itree.h"

namespace cgc {

  typedef u_int8_t memptr_t;

  class untyped_bigptr_t;

  //=======================================================================

  class memslot_t {
  public:

    //
    // Make sure these 3 elements are all aligned, which they are
    // since all are pointers and/or size_t's.  Might want to revisit
    // this assumption.
    //
    // Also note that the next three fields (4 long's worth) are all used
    // during compaction.  For this reason, we might consider alternate
    // storage for object that are much smaller and therefore don't need
    // to be compacted.
    //
    tailq_entry<memslot_t> _next;
    untyped_bigptr_t *_ptrslot;
    size_t _sz;

    memptr_t _data[0];

    static size_t size (size_t s) { return sizeof (memslot_t) + s; }
    size_t size () const { return size (_sz); }

    memptr_t *v_data () { return _data; }
    const memptr_t *v_data () const { return _data; }
    void reseat ();

    void copy (const memslot_t *ms);
    void slotfree ();

  };

  //=======================================================================

  typedef tailq<memslot_t, &memslot_t::_next> memslot_list_t;

  //=======================================================================

  class untyped_bigptr_t {
  public:
    untyped_bigptr_t (memslot_t *m) : _ms (m), _count (1) {}
    void set_mem_slot (memslot_t *ms) { _ms = ms; }
    memslot_t *memslot () const { return _ms; }
    void init (memslot_t *m, int c)  { _ms = m; _count = c; }
    void mark_free () { _count = -1; }
    int count () const { return _count; }
  protected:
    memslot_t *_ms;
    int _count;
  };

  //=======================================================================

  template<class T>
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

    T *obj () 
    { 
      assert (count() > 0);
      return reinterpret_cast<T *> (v_data ()); 
    }

    const T *obj () const 
    { 
      assert (count() > 0);
      return reinterpret_cast<const T *> (v_data ()); 
    }

    const T *lim () const
    {
      assert (count() > 0);
      return reinterpret_cast<const T *> (v_data() + size());
    }

    void rc_inc () { set_count (count () + 1); }

    bool rc_dec () {
      bool ret;

      int c = count ();
      assert (c > 0);
      if (--c == 0) {
	obj ()->~T();
	slotfree ();
	ret = false;
      } else {
	ret = true;
      }
      set_count (c);
      return ret;
    }
  };

  //=======================================================================

  template<class T>
  class bigptr_t : public virtual redirect_ptr_t<T>, 
		   public untyped_bigptr_t {
  public:
    bigptr_t (memslot_t *m) : untyped_bigptr_t (m) {}
    int count () const { return untyped_bigptr_t::count (); }
    void set_count (int i) { _count = i; }
    size_t size () const { return _ms->size (); }
    memptr_t *v_data () { return _ms->v_data (); }
    const memptr_t *v_data () const { return _ms->v_data (); }
    void slotfree () { _ms->slotfree (); }

  };

  //=======================================================================

  template<class T> class allocator_t;

  //=======================================================================

  template<class T>
  class ptr {
  public:
    ptr () : _redir_ptr (NULL) {}
    virtual ~ptr () { rc_dec(); }

    const T *volatile_ptr () const 
    {
      if (_redir_ptr) { return _redir_ptr->obj (); }
      else return NULL;
    }

    T *volatile_ptr ()
    {
      if (_redir_ptr) { return _redir_ptr->obj (); }
      else return NULL;
    }

    void rc_inc ()
    {
      if (_redir_ptr) { _redir_ptr->rc_inc (); }
    }

    bool rc_dec ()
    {
      if (_redir_ptr) { return _redir_ptr->rc_dec (); }
      else return false;
    }

    const T *nonnull_volatile_ptr () const 
    {
      const T *ret = volatile_ptr ();
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

    ptr<T> &operator= (ptr<T> &p)
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


    friend class allocator_t<T>;
  protected:
    explicit ptr (redirect_ptr_t<T> *p) : _redir_ptr (p) { rc_inc (); }
    bool base_eq (const ptr<T> &p) { return _redir_ptr == p._redir_ptr; }
  private:
    virtual void v_clear () {}

    redirect_ptr_t<T> *_redir_ptr;
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
      return (ptr<T>::_redir_ptr->obj() + _offset <= 
	      ptr<T>::_redir_ptr->lim());
    }

  protected:
    aptr (redirect_ptr_t<T> *p, size_t s) : ptr<T> (p), _offset (s) {}

  private:

    T *obj ()
    {
      assert (inbounds ());
      return (ptr<T>::_redir_ptr->obj() + _offset);
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
    
  private:
    T *_base;
    size_t _nxt, _size;
  };

  //=======================================================================

  class arena_t {
  public:
    arena_t (memptr_t *base, size_t sz)  { init (base, sz); }
    arena_t ()
      : _base (NULL), _top (NULL), _nxt_ptrslot (NULL), _nxt_memslot (NULL),
	_sz (0) {}

    void init (memptr_t *base, size_t sz);

    virtual ~arena_t ();

    untyped_bigptr_t *alloc (size_t sz);

    template<class T> bigptr_t<T> *alloc ()
    {
      untyped_bigptr_t *vp = alloc (sizeof (T));
      return reinterpret_cast<bigptr_t<T> *> (vp);
    }

    void register_arena (void);
    void unregister_arena (void);

    int cmp (const memptr_t *m) const;

    void gc (void);

    // make a tree of all active arenas, so that we can take an
    // object and figure out which arena it lives in.
    itree_entry<arena_t> _tlnk;
    memptr_t *_base;

    tailq_entry<arena_t> _qlnk;

    static arena_t *pick_arena (size_t sz);

    void remove (memslot_t *m) { _memslots.remove (m); }

  protected:
    untyped_bigptr_t *get_free_ptrslot (void);
    void collect_ptrslots (void);
    void compact_memslots (void);

    memptr_t *_top;
    memptr_t *_nxt_ptrslot;
    memptr_t *_nxt_memslot;
    size_t _sz;

    memslot_list_t _memslots;
    simple_stack_t<untyped_bigptr_t *> _free_ptrslots;
  };

  //=======================================================================

  template<class T>
  class allocator_t {
  public:
    allocator_t (arena_t *a = NULL) : _arena (a) {}

    VA_TEMPLATE(ptr<T> alloc,						\
		{ arena_t *a = _arena ? _arena :			\
		    arena_t::pick_arena (sizeof (T));			\
		  bigptr_t<T> *ret = a->alloc<T>();			\
		  (void) new (ret->v_data ()) T ,			\
		    ; return ptr<T> (ret); } )

  private:
    arena_t *_arena;
  };

  class mmap_arena_t : public arena_t {
  public:
    mmap_arena_t (size_t npages);
    ~mmap_arena_t ();
    
    static size_t pagesz;
  };

  //=======================================================================

  class arena_set_t {
  public:
    arena_set_t () : _p (NULL) {}
    void insert (arena_t *a);
    void remove (arena_t *a);
    arena_t *pick (size_t sz);

    template<class T> arena_t *
    lookup (ptr<T> p) { return v_lookup (p->memslot ()->v_data ()); }

    arena_t *
    lookup (const memslot_t *m) 
    { return v_lookup (reinterpret_cast<const memptr_t *> (m)); }

  private:
    arena_t *v_lookup (const memptr_t *p);

    arena_t *_p;
    tailq<arena_t, &arena_t::_qlnk> _order;
    itree<memptr_t *, arena_t, &arena_t::_base, &arena_t::_tlnk> _tree;
  };

  extern arena_set_t g_arena_set;

  //=======================================================================

  void
  memslot_t::slotfree ()
  {
    arena_t *a = g_arena_set.lookup (this);
    if (a) {
      a->remove (this);
    }
  }

  //=======================================================================

};
