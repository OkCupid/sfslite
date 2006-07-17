
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#include "pipeline.h"

TAMED cbv::ptr
pipeliner_void_t::make_cb (size_t i)
{
  VARS { cbv::ptr ret; }
  DEFAULT_RETURN { return ret; }
  ret = @[_cg]();
}

class pipeliner_cb_t : public pipeliner_void_t {
public:
  pipeliner_cb_t (size_t w, size_t n, pipeline_op_t o)
    : pipeliner_void_t (w, n), _op (o) {}
  void pipeline_op (size_t i, cbv cb, CLOSURE);
private:
  pipeline_op_t _op;

};

TAMED void
pipeliner_cb_t::pipeline_op (size_t i, cbv cb)
{
  VARS {
    bool ok;
  }
  BLOCK { (*_op) (i, @(ok), NULL); }
  if (!ok) cancel ();
  cb->signal ();
}

TAMED void
do_pipeline (size_t w, size_t n, pipeline_op_t op, cbv done)
{
  VARS {
    pipeliner_cb_t ppl (w, n, op);
  }
  BLOCK { ppl.run (@()); }
  done->signal ();
}
