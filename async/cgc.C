
#include "cgc.h"

namespace cgc {

  void memslot_t::reseat () { _ptrslot->set_mem_slot (this); }

  v_ptrslot_t *
  arena_t::get_free_ptrslot ()
  {
    v_ptrslot_t *ret = NULL;
    if (_free_ptrslots.size ()) {
      ret = _free_ptrslots.pop_back ();
    } else {
     ret = reinterpret_cast<v_ptrslot_t *> (_nxt_ptrslot);
     _nxt_ptrslot += sizeof (*ret);
    }
    return ret;
  }

  v_ptrslot_t *
  arena_t::alloc (size_t sz)
  {
    v_ptrslot_t *res = NULL;
    if (memslot_t::size (sz) + _nxt_memslot <= _nxt_ptrslot) {
      memslot_t *ms = reinterpret_cast<memslot_t *> (_nxt_memslot);
      v_ptrslot_t *ps = reinterpret_cast<v_ptrslot_t *> (_nxt_ptrslot);
      
      ms->_sz = sz;
      ms->_ptrslot = ps;

      ps->init (ms, 1);

      _nxt_memslot += ms->size ();
      _nxt_ptrslot += sizeof (*ps);
      _memslots.insert_tail (ms);
    }
    return res;
  }

  template<class T>
  ptrslot_t<T> *
  arena_t::alloc ()
  {
    v_ptrslot_t *v = alloc (sizeof (T));
    return reinterpret_cast<ptrslot_t<T> *> (v);
  }
  
  void
  arena_t::collect_ptrslots (void)
  {
    v_ptrslot_t *p = reinterpret_cast<v_ptrslot_t *> (_top) - 1;
    v_ptrslot_t *last = reinterpret_cast<v_ptrslot_t *> (_nxt_ptrslot) + 1;
    v_ptrslot_t *last_used = NULL;

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
      _nxt_ptrslot = reinterpret_cast<u_int8_t *> (last - 1);
      while (_free_ptrslots.size () && _free_ptrslots.back () > last)
	_free_ptrslots.pop_back ();
    }

  }

  void
  arena_t::compact_memslots (void)
  {
    u_int8_t *p = _base;
    memslot_t *m = _memslots.first;
    memslot_t *n = NULL;

    memslot_list_t nl;
    
    while (m) {
      n = _memslots.next (m);
      _memslots.remove (m);
      memslot_t *ns = reinterpret_cast<memslot_t *> (p);
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

  void
  arena_t::gc (void)
  {
    collect_ptrslots ();
    compact_memslots ();
  }


};
