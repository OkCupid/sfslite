
#include "cgc.h"

namespace cgc {

  void memslot_t::reset () { _ptrslot->set_mem_slot (this); }

  v_ptrslot_t *
  arena_t::alloc (size_t sz)
  {
    v_ptrslot_t *res = NULL;
    if (memslot_t::size (sz) + _nxt_memslot <= _nxt_ptrslot) {
      memslot_t *ms = reinterpret_cast<memslot_t *> (_nxt_memslot);
      v_ptrslot_t *ps = reinterpret_cast<v_ptrslot_t *> (_nxt_ptrslot);
      ms->_sz = sz;
      ms->_ptrslot = ps;
      _ps->_ms = ms;
      _ps->_count = 1;

      _nxt_memslot += ms->size ();
      _nxt_ptrslot += sizeof (*ps);
      _memslots.insert_head (ms);
    }
    return res;
  }

  ptrslot_t<T> *
  arena_t::alloc ()
  {



  }


};
