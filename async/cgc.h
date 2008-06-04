
// -*- c++ -*-

#include "refcnt.h"
#include "callback.h"
#include "list.h"
#include "vec.h"
#include "itree.h"

namespace cgc {

  typedef u_int8_t memptr_t;

  class v_ptrslot_t;

  //=======================================================================

  class memslot_t {
  public:

    // Make sure these 3 elements are all aligned, which they are
    // since all are pointers and/or size_t's.  Might want to revisit
    // this assumption.
    tailq_entry<memslot_t> _next;
    v_ptrslot_t *_ptrslot;
    size_t _sz;

    memptr_t _data[0];

    static size_t size (size_t s) { return sizeof (memslot_t) + s; }
    size_t size () const { return size (_sz); }

    memptr_t *v_data () { return _data; }
    const memptr_t *v_data () const { return _data; }
    void reseat ();

    template<class T> T *
    data () { return reinterpret_cast<T *> (_data); }
    template<class T> const T *
    data () const { return reinterpret_cast<T *> (_data); }

    template<class T> T *
    lim () { return reinterpret_cast<T *> (_data + _sz); }
    template<class T> const T *
    lim () const { return reinterpret_cast<T *> (_data + _sz); }

    void
    copy (const memslot_t *ms)
    {
      _ptrslot = ms->_ptrslot;
      memcpy (_data, ms->_data, ms->_sz);
      _sz = ms->_sz;
    }

    template<class T> void finalize ();
  };

  //=======================================================================

  typedef tailq<memslot_t, &memslot_t::_next> memslot_list_t;

  //=======================================================================

  class v_ptrslot_t {
  public:
    v_ptrslot_t (memslot_t *m) : _ms (m), _count (1) {}
    void set_mem_slot (memslot_t *ms) { _ms = ms; }
    memslot_t *memslot () const { return _ms; }
    void init (memslot_t *m, int c)  { _ms = m; _count = c; }
    int count() const { return _count; }
    void mark_free () { _count = -1; }
  protected:
    memslot_t *_ms;
    int _count;
  };

  //=======================================================================

  template<class T>
  class ptrslot_t : public v_ptrslot_t {
  public:
    ptrslot_t (memslot_t *m) : v_ptrslot_t (m) {}

    T *obj () 
    { 
      assert (_count > 0); 
      return _ms->data<T> (); 
    }
    
    const T *obj () const
    {
      assert (_count > 0);
      return _ms->data<T> (); 
    }

    T *lim () const
    {
      assert (_count > 0);
      return _ms->lim<T> ();
    }

    void rc_inc () { _count++; }

    bool rc_dec () {
      bool ret;

      assert (_count > 0);
      --_count;
      if (_count == 0) {
	_ms->finalize<T> ();
	ret = false;
      } else {
	ret = true;
      }
      return ret;
    }

  };

  //=======================================================================

  template<class T> class allocator_t;

  template<class T>
  class ptr {
  public:
    ptr () : _ptrslot (NULL) {}
    ~ptr () { if (_ptrslot) _ptrslot->rc_dec (); }

    T *volatile_ptr () const
    {
      if (!_ptrslot) return NULL;
      else return obj ();
    }

    const T *operator-> () const { assert (_ptrslot); return obj(); }
    T *operator-> () { assert (_ptrslot); return obj(); }
    T &operator* () const { assert (_ptrslot); return obj(); }

    ptr<T> &operator= (ptr<T> &p)
    {
      if (_ptrslot) _ptrslot->rc_dec ();
      _ptrslot = p._ptrslot;
      v_clear ();
      if (_ptrslot) _ptrslot->rc_inc ();
      return (*this);
    }

    ptr<T> &operator= (int i)
    {
      assert (i == 0);
      if (_ptrslot) _ptrslot->rc_dec ();
      v_clear ();
      return (*this);
    }


    bool operator== (const ptr<T> &p) const { return _ptrslot == p._ptrslot; }
    bool operator!= (const ptr<T> &p) const { return ! ( (*this) == p); }
    
    friend class allocator_t<T>;
  protected:
    explicit ptr (ptrslot_t<T> *p) : _ptrslot (p) { if (p) p->rc_inc (); }
  private:
    virtual T *obj () { return _ptrslot->obj (); }
    virtual void v_clear () {}
    ptrslot_t<T> *_ptrslot;
  };

  //=======================================================================

  template<class T>
  class aptr : public ptr<T> {
  public:
    aptr () : ptr<T> (), _offset (0) {}
    aptr (size_t s) : ptr<T> (), _offset (s) {}
    
    bool operator== (const aptr<T> &p) const
    { return _ptrslot == p._ptrslot && _offset = p._offset;  }

    bool operator== (const aptr<T> &p) const
    { return ! ( (*this) == p); }
    
    aptr<T> &operator= (aptr<T> &p)
    {
      if (_ptrslot) _ptrslot->rc_dec ();
      _ptrslot = p._ptrslot;
      _offset = p._offset;
      if (_ptrslot) _ptrslot->rc_inc ();
      return (*this);
    }

    aptr<T> operator+ (size_t s) const
    {
      aptr<T> ret (_ptrslot, _offset + s);
      assert (ret.inbounds ());
      return ret;
    }

    aptr<T> operator- (size_t s) const
    {
      assert (s <= _offset);
      aptr<T> ret (_ptrslot, _offset - s);
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
      assert (_ptrslot == p._ptrslot);
      return (_offset < p._offset); 
    } 

    bool operator<= (const aptr<T> &p) const 
    { 
      assert (_ptrslot == p._ptrslot);
      return (_offset <= p._offset); 
    }
 
    bool operator>= (const aptr<T> &p) const 
    { 
      assert (_ptrslot == p._ptrslot);
      return (_offset >= p._offset); 
    } 

    bool operator> (const aptr<T> &p) const 
    { 
      assert (_ptrslot == p._ptrslot);
      return (_offset > p._offset); 
    } 

    bool inbounds ()
    {
      return (_ptrslot->obj() + _offset <= _ptrslot->lim());
    }

  protected:
    aptr (ptrslot_t<T> *p, size_t s) : ptr<T> (p), _offset (s) {}

  private:

    T *obj ()
    {
      assert (inbounds ());
      return (_ptrslot->obj() + _offset);
    }

    void v_clear () { _offset = 0; }

    ptrslot_t<T> *_ptrslot;
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

    v_ptrslot_t *alloc (size_t sz);

    template<class T> ptrslot_t<T> *alloc ()
    {
      v_ptrslot_t *vp = alloc (sizeof (T));
      return reinterpret_cast<ptrslot_t<T> *> (vp);
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
    v_ptrslot_t *get_free_ptrslot (void);
    void collect_ptrslots (void);
    void compact_memslots (void);

    memptr_t *_top;
    memptr_t *_nxt_ptrslot;
    memptr_t *_nxt_memslot;
    size_t _sz;

    memslot_list_t _memslots;
    simple_stack_t<v_ptrslot_t *> _free_ptrslots;
  };

  //=======================================================================

  template<class T>
  class allocator_t {
  public:
    allocator_t (arena_t *a = NULL) : _arena (a) {}

    VA_TEMPLATE(ptr<T> alloc,						\
		{ arena_t *a = _arena ? _arena :			\
		    arena_t::pick_arena (sizeof (T));			\
		  ptrslot_t<T> *ret = a->alloc<T>();			\
		  (void) new (ret->memslot ()->v_data ()) T ,		\
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

  template<class T> void
  memslot_t::finalize ()
  {
    data<T> ()->~T();
    arena_t *a = g_arena_set.lookup (this);
    if (a) {
      a->remove (this);
    }
  }

  //=======================================================================

};
