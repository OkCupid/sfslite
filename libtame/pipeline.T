
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#include "pipeline.h"

TAMED void
pipeliner_t::wait_n (size_t n, cbv done)
{
  while (_cg.n_signals_left () > n) {
    WAIT (_cg);
  }
  done->signal ();
}

TAMED void
pipeliner_t::launch (size_t i, cbv done)
{
  BLOCK { wait_n (_wsz - 1, @()); }
  pipeline_op (i, @[_cg]() );
  done->signal ();
}

TAMED void
pipeliner_t::run (cbv done)
{
  VARS {
    size_t i;
  }
  for (i = 0; i < keep_going (i) && !_cancelled; i++) {
    BLOCK { launch (i, @()); }
  }
  BLOCK { wait_n (0, @()); }
  done->signal ();
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
