// -*-c++-*-
/* $Id: ex1.T 2236 2006-09-29 00:00:22Z max $ */

#include "async.h"
#include "tame.h"
#include <list.h>

struct req_t {
  req_t (int p, cbi::ptr d) : _param (p), _done (d) {}
  int _param;
  cbi::ptr _done;
};

struct worker_t {
  worker_t () : _req (NULL) {}
  void serve ();
  int action (int i);
  int _index;
  list_entry<worker_t> _rlnk;
  cbv::ptr _ev;
  bool _shutdown;
  req_t *_req;
};

struct server_t {
public:
  server_t (size_t n) : _n_threads (n) { init (); }
  void run (cbv cb, CLOSURE);
  void shutdown (cbv cb, CLOSURE);
  void serve (int i, cbi cb, CLOSURE);
private:
  void init ();
  void ready_thread (worker_t *w);
  size_t _n_threads;
  vec<worker_t> _workers;
  list<worker_t, &worker_t::_rlnk> _ready;
  size_t _n_alive;
  vec<cbv> _waiters;
  rendezvous_t<size_t> _rv;
  cbv::ptr _shutdown_cb;
  bool _shutdown;
};

void
server_t::init ()
{
  _workers.setsize (_n_threads);

  for (size_t i = 0; i < _workers.size (); i++) {
    worker_t *w = &_workers[i];
    w->_index = i;
    cfork (_rv, i, wrap (w, &worker_t::serve));
    ready_thread (w);
  }
}

void
server_t::ready_thread (worker_t *w)
{
  _ready.insert_head (w);
  if (_waiters.size ()) {
    cbv cb = _waiters.pop_front ();
    cb->signal ();
  }
}

tamed void
server_t::run (cbv cb)
{
  cwait { _shutdown_cb = mkevent (); }
  cb->signal ();
}

tamed void
server_t::shutdown (cbv cb)
{
  cvars {
    size_t i;
  }
  warn << "s> shutdown started...\n";
  _shutdown = true;
  for (i = 0; i < _workers.size (); i++) {
    worker_t *w = &_workers[i];
    w->_shutdown = true;
    if (w->_ev) w->_ev->signal ();
  }
  while (_rv.n_out ()) {
    cwait (_rv, i);
    warn << "s> thread " << i << " exitted\n";
  }
  warn << "s> shutdown finished...\n";
  cb->signal ();
}

void
worker_t::serve ()
{
  bool go = true;
  int res;
  warn << "w> " << _index << " starting up...\n";
  while (!_shutdown) {
    if (!_req) {
      cwait { _ev = mkevent (); }
      _ev = NULL;
    }
    if (_req) {
      warn << "w> action (" << _req->_param << ")...\n";
      int i = action (_req->_param);
      warn << "w> action (" << _req->_param << ") signalling...\n";
      _req->_done->signal (i);
      _req = NULL;
    }
  }
  warn << "w> " << _index << " shutting down...\n";
}

int
worker_t::action (int i)
{
  int s = rand () % 10 + 1;
  warn << _index << ": server req=" << i <<  "; "
       << "sleep=" << s << "\n";
  sleep (s);
  warn << _index << ": wake up\n";
  return s;
}

tamed void
server_t::serve (int i, cbi cb)
{
  cvars {
    worker_t *w;
    int res (-1);
    req_t *r;
  }
  if (i == 0) {
    if (_shutdown_cb) {
      warn << "s> invoking shutdown_cb ...\n";
      cbv c = _shutdown_cb;
      _shutdown_cb = NULL;
      c->signal ();
    }
  } else if (!_shutdown) {
    while (!(w = _ready.first)) {
      cwait { _waiters.push_back (mkevent ()); }
    }
    _ready.remove (w);
    if (!w->_shutdown) {
      cwait { 
	r = New req_t (i, mkevent (res));
	w->_req = r;
	if (w->_ev) 
	  w->_ev->signal ();
      }
      warn ("Delete r=%p\n", r);
      delete r;
    }
  }
  cb->signal (res);
}

tamed static void
run_server (server_t **sp, cbv cb) 
{
  cvars {
    server_t *s (New server_t (4));
  }
  *sp = s;

  cwait { s->run (mkevent ()); }
  warn << "m> server done running...\n";
  cwait { s->shutdown (mkevent ()); }
  warn << "m> server done shutting down ...\n";
  delete s;
  cb->signal ();
}

#define N_CALLS 100

tamed static void
run_client (server_t *s, cbv cb)
{
  cvars {
    int res[N_CALLS];
    int i;
    rendezvous_t<int> rv;
    int sdres;
  }

  for (i = 0; i < N_CALLS; i++) {
    s->serve (i+1, mkevent (rv, i, res[i]));
    cwait { delaycb (2, 0, mkevent ()); }
  }
  while (rv.n_out ()) {
    cwait (rv, i);
    warn << "c> thread " << i << " returned: " << res[i] << "\n";
  }
  warn << "c> shutdown signal\n";
  cwait { s->serve (0, mkevent (sdres)); }
  cb->signal ();
}

#undef N_CALLS
  
tamed static 
void main2 ()
{
  cvars {
    server_t *s;
  }
  cwait {
    run_server (&s, mkevent ());
    run_client (s, mkevent ());
  }
}


int main (int argc, char *argv[])
{
  pth_init ();
  main2 ();
  amain ();
}
