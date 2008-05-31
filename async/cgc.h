
namespace cgc {

  class v_ptrslot_t;

  typedef tailq<memslot_t, &memslot_t::_next> memslot_list_t;

  class memslot_t {
  public:
    tailq_entry<memslot_t> _next;
    v_ptrslot_t *_ptrslot;
    size_t _sz;
    u_int8_t _data[0];

    static size (size_t s) { return sizeof (memslot_t) + s; }
    size_t size () const { return size (_sz); }

    u_int8_t *v_data () const { return _data; }
    void reseat ();

    template<class T> T *
    data () const { return reinterpret_cast<T *> (_data); }

    template<class T> T *
    lim () const { return reinterpret_cast<T *> (_data + s); }

    void
    copy (const memslot_t *ms)
    {
      _ptrslot = ms->_ptrslot;
      memcpy (_data, ms->_data, ms->_sz);
      _sz = ms->sz;
    }

    template<class T> void
    finalize ()
    {
      data<T> ()->~T();
      memslot_list_t::remove (this);
    }

  };

  class v_ptrslot_t {
  public:
    v_ptrslot_t (memslot_t *m) : _ms (m), _count (1) {}
    void set_mem_slot (memslot_t *ms) { _ms = ms; }
    memslot_t *memslot () const { return _ms; }
  protected:
    memslot_t *_ms;
    u_int _count;
  };

  template<class T>
  class ptrslot_t : v_ptrslot_t {
  public:
    ptrslot (memslot_t *m) : v_ptrslot_t (m) {}

    T *obj () const 
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

  private:
    void *_obj;
  };

  template<class T>
  class ptr {
  public:
    ptr () : _ptrslot (NULL) {}
    ~ptr () { if (_ptrslot) _ptrslot->rc_dec (); }

    T *volatile_ptr () const
    {
      if (!_ptr_slot) return NULL;
      else return obj ();
    }

    T *operator-> () const { assert (_ptrslot); return obj(); }
    T &operator* () const { assert (_ptrslot); return obj(); }

    ptr<T> &operator= (ptr<T> &p)
    {
      if (_ptrslot) _ptrslot->rc_dec ();
      _ptrslot = p._ptrslot;
      v_clear ();
      if (_ptrslot) _ptrslot->rc_inc ();
      return (*this);
    }

    bool operator== (const ptr<T> &p) const { return _ptrslot == p._ptrslot; }
    bool operator== (const ptr<T> &p) const { return ! ( (*this) == p); }

  protected:
    ptr (ptrslot_t<T> *p) : _ptrslot (p) { if (p) p->rc_inc (); }
    
  private:
    virtual T *obj () { return _ptrslot->obj<T> (); }
    virtual void v_clear () {}
    ptrslot_t<T> *_ptrslot;
  };


  template<class T>
  class aptr : class ptr<T> {
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

    aptr<T> operator++ (size s) { return (*this) += 1; }
    aptr<T> operator-- (size s) { return (*this) -= 1; }

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

#define RELOP(op) \
    bool operator##op (const aptr<T> &p) const \
    { \
      assert (_ptrslot == p._ptrslot); \
      return (_offset op p._offset);  \
    } 
    RELOP(<)
    RELOP(<=)
    RELOP(>)
    RELOP(>=)
#undef RELOP

    bool inbounds ()
    {
      return (_ptrslot->obj<T>() + _offset <= _ptrslot->lim<T> ());
    }

  protected:
    aptr (ptrslot_t<T> *p, size_t s) : ptr<T> (p), _offset (s) {}

  private:

    T *obj ()
    {
      assert (inbounds ());
      return (_ptrslot->obj<T> + _offset);
    }

    void v_clear () { _offset = 0; }

    ptrslot_t<T> *_ptrslot;
    size_t _offset;
  };

  class arena_t {
  public:
    arena_t (u_int8_t *base, size_t sz) 
      : _base (base),
	_top (_base + sz),
	_nxt_ptrslot (_base),
	_next_memslot (_top - sizeof (memslot_t)),
	_sz (sz)
    {}

    v_ptrslot_t *alloc (size_t sz);

    void gc (void);

  private:
    u_int8_t *_base;
    u_int8_t *_top;
    u_int8_t *_nxt_ptrslot;
    u_int8_t *_next_memslot;
    size_t _sz;

    memslot_list_t _memslots;
    vec<v_ptrslot_t *> _free_ptr_slots;
  };

};
