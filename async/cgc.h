
namespace cgc {

  class v_ptrslot_t;

  typedef memslot_list_t<memslot_t, &memslot_t::_next> memslot_list_t;

  class memslot_t {
  public:
    list_entry<memslot_t> _next;
    v_ptrslot_t *_ptrslot;
    size_t _sz;
    u_int8_t _data[0];

    static size (size_t s) { return sizeof (memslot_t) + s; }
    size_t size () const { return size (_sz); }

    u_int8_t *v_data () const { return _data; }
    void reseat ();

    template<class T> T *
    data () const { return reinterpret_cast<T *> (_data); }

    void
    copy (memslot_t *ms)
    {
      memcpy (_data, ms->_data, ms->_sz);
      _sz = ms->sz;
    }

    template<class T> void
    finalize ()
    {
      data<T> ()->~();
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
  class ptr {
  public:

    ptr () { _ptrslot = NULL; }
    ~ptr () { if (_ptrslot) _ptrslot->rc_dec (); }

    T *operator-> () const { assert (_ptrslot); return _ptrslot->obj <T> (); }
    T &operator* () const { assert (_ptrslot); return *_ptrslot->obj <T> (); }

    ptr<T> &operator= (ptr<T> &p)
    {
      if (_ptrslot) _ptrslot->rc_dec ();
      _ptrslot = p._ptrslot;
      if (_ptrslot) _ptrslot->rc_inc ();
      return (*this);
    }
    
  private:
    ptrslot_t<T> *_ptrslot;
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

  private:
    u_int8_t *_base;
    u_int8_t *_top;
    u_int8_t *_nxt_ptrslot;
    u_int8_t *_next_memslot;
    size_t _sz;

    memslot_list_t _memslots;
  };

};
