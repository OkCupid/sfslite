
#include "tame_closure.h"


bool
closure_t::block_dec_count (const char *loc)
{
  bool ret = false;
  if (_block._count <= 0) {
    tame_error (loc, "too many triggers for wait environment.");
  } else if (--_block._count == 0) {
    ret = true;
  }
  return ret;
}

str
closure_t::loc (int l) const
{
  strbuf b;
  b << _filename << ":" << l << " in function " << _funcname;
  return b;
}

closure_t::closure_t (const char *file, const char *fun)
  : weak_refcounted_t<closure_t> (this),
    _jumpto (0), 
    _id (++closure_serial_number),
    _filename (file),
    _funcname (fun),
    _must_deallocate_list (must_deallocate_list_t::alloc ()) 
  {}

void
closure_t::error (int lineno, const char *msg)
{
  str s = loc (lineno);
  tame_error (s.cstr(), msg);
}

void
closure_t::init_block (int blockid, int lineno)
{
  _block._id = blockid;
  _block._count = 1;
  _block._lineno = lineno;
}


void
closure_t::report_rv_problems ()
{
  for (u_int i = 0; i < _rvs.size (); i++) {
    if (_rvs[i]->is_alive ()) {
      tame_error (_rvs[i]->pointer ()->loc (),
		  "rendezvous still active after control left function");
    }
  }
}
