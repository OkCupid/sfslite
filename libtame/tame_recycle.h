

// -*-c++-*-
/* $Id: tame_event.h 2487 2007-01-09 14:54:32Z max $ */

#ifndef _LIBTAME_RECYCLE_H_
#define _LIBTAME_RECYCLE_H_

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
  enum { defsz = 8192 };
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

INIT(recycle_init);

typedef enum { OBJ_ALIVE = 0, 
	       OBJ_SICK = 0x1, 
	       OBJ_DEAD = 0x2,
	       OBJ_CANCELLED = 0x4 } obj_state_t;

class obj_flag_t : virtual public refcount {
public:
  obj_flag_t (const obj_state_t &b) : _flag (b), _can_recycle (true) {}
  ~obj_flag_t () { }

  static void recycle (obj_flag_t *p);
  static ptr<obj_flag_t> alloc (const obj_state_t &b);
  static recycle_bin_t<obj_flag_t> *get_recycle_bin ();

  void finalize () { if (_can_recycle) recycle (this); }
  inline bool get (obj_state_t b) { return (_flag & b) == b; }

  inline void set (obj_state_t b) 
  { _flag = obj_state_t (int (_flag) | b); }

  void set_can_recycle (bool b) { _can_recycle = b; }

  inline bool is_alive () const { return _flag == OBJ_ALIVE; }
  inline bool is_cancelled () const { return (_flag & OBJ_CANCELLED); }
  inline void set_dead () { set (OBJ_DEAD); }
  inline void set_cancelled () { set (OBJ_CANCELLED); }
private:
  obj_state_t _flag;
  bool _can_recycle;
};

typedef ptr<obj_flag_t> obj_flag_ptr_t;

#endif /* _LIBTAME_RECYCLE_H_ */
