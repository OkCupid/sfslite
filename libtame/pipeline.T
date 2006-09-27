
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#include "pipeline.h"

pipeliner_t::pipeliner_t (size_t w) : 
  _wsz (w), _cg (__FILE__, __LINE__), _cancelled (false)
{ assert (_wsz > 0); }

TAMED void
pipeliner_t::wait_n (size_t n, cbv done)
{
  while (_cg.n_signals_left () > n) {
    WAIT (_cg);
  }
  SIGNAL (done);
}

TAMED void
pipeliner_t::launch (size_t i, cbv done)
{
  BLOCK { wait_n (_wsz - 1, mkevent0 () ); }
  pipeline_op (i, mkevent (_cg) );
  SIGNAL (done);
}

TAMED void
pipeliner_t::run (cbv done)
{
  VARS {
    size_t i;
  }
  for (i = 0; keep_going (i) && !_cancelled; i++) {
    BLOCK { launch (i, mkevent0 () ); }
  }
  BLOCK { wait_n (0, mkevent0 () ); }
  SIGNAL (done);
}

class pipeliner_cb_t : public pipeliner_t {
public:
  pipeliner_cb_t (size_t w, size_t n, pipeline_op_t o)
    : pipeliner_t (w), _n_calls (n), _op (o) {}

protected:
  void pipeline_op (size_t i, cbv cb, CLOSURE);
  bool keep_going (size_t i) const { return i < _n_calls; }

private:
  size_t _n_calls;
  pipeline_op_t _op;

};

TAMED void
pipeliner_cb_t::pipeline_op (size_t i, cbv cb)
{
  VARS {
    bool ok;
  }
  BLOCK { (*_op) (i, mkevent (ok), NULL); }
  if (!ok) cancel ();
  SIGNAL (cb);
}

TAMED void
do_pipeline (size_t w, size_t n, pipeline_op_t op, cbv done)
{
  VARS {
    pipeliner_cb_t ppl (w, n, op);
  }
  BLOCK { ppl.run (mkevent0 ()); }
  SIGNAL (done);
}
