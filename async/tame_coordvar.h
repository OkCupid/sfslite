
#ifndef _COORDVAR_H_INCLUDED
#define _COORDVAR_H_INCLUDED 1

#include "refcnt.h"
#include "vec.h"
#include "init.h"

template<class C>
class container_t {
public:
  container_t (const C &c) { (void) new (getp_void ()) C (c); }
  inline void *getp_void () { return reinterpret_cast<void *> (_bspace); }
  inline C *getp () { return reinterpret_cast<C *> (getp_void ()); }
  void del () { getp ()->~C (); }
  C &get () { return *getp (); }
  const C &get () const { return *getp (); }
private:
  char _bspace[sizeof(C)];
};

/**
 * A class for collecting recycled objects, and recycling them.  Different
 * from a regular vec in that we never give memory back (via pop_back)
 * which should save some shuffling.
 *
 * Required: that T inherits from refcnt
 */
template<class T>
class recycle_bin_t {
public:
  enum { defsz = 1024 };
  recycle_bin_t (size_t s = defsz) : _capacity (s), _cursor (0)  {}
  void expand (size_t s) { if (_capacity < s) _capacity = s; }
  bool add (T *obj)
  {
    int nobj = 1;
    if (_cursor < _objects.size ()) {
      _objects[_cursor] = mkref (obj);
    } else if (_objects.size () < _capacity) {
      _objects.push_back (mkref (obj));
    } else {
      nobj = 0;
    }
    _cursor += nobj;
    return nobj;
  }
  ptr<T> get () 
  {
    ptr<T> ret;
    if (_cursor > 0) {
      ret = _objects[--_cursor];
      _objects[_cursor] = NULL;
    }
    return ret;
  }
private:
  vec<ptr<T> > _objects;
  size_t       _capacity;
  size_t       _cursor;
};

INIT(ref_flag_init);

/**
 * ref_flag_t
 *
 * Kind of like ptr<bool>'s, but are recycled for performance.
 */
class ref_flag_t : virtual public refcount {
public:
  ref_flag_t (const bool &b) : _flag (b), _can_recycle (true) {}
  ~ref_flag_t () { }

  static void recycle (ref_flag_t *p);
  static ptr<ref_flag_t> alloc (const bool &b);
  static recycle_bin_t<ref_flag_t> *get_recycle_bin ();

  void finalize () { if (_can_recycle) recycle (this); }
  operator const bool &() const { return _flag; }
  bool get () const { return _flag; }
  void set (bool b) { _flag = b; }
  void set_can_recycle (bool b) { _can_recycle = b; }
private:
  bool _flag;
  bool _can_recycle;
};

typedef ptr<ref_flag_t> ref_flag_ptr_t;

// In strict mode, panic on a second signal.
extern bool coordvar_strict_mode;

void coordvar_second_signal (const char *file, const char *line);

#endif /* _COORDVAR_H_INCLUDED */
