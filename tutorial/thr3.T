// -*-c++-*-
/* $Id: ex1.T 2236 2006-09-29 00:00:22Z max $ */

#include "tame.h"
#include <list.h>

struct req_t {
  req_t (int t, cbv::ptr d) : _time (t), _done (d); {}
  int _time;
  cbv::ptr _done;
};

struct worker_t {
  int _index;
  rendezvous_t<req_t> _rv;
  list_entry<worker_t> _rlnk;
  bool _dead;
};

class action_t;

struct app_state_t {
  app_state_t () 
  {
    _workers.setsize (10);
    _n_alive  = _workers.size ();
  }
    
  vec<worker_t> _workers;
  list<worker_t, &worker_t::_rlink> _ready;
  bool _client_alive;
  size_t _n_alive;
  vec<int> _pending;
  rendezvous_t<ptr<action_t> > _rv;
};

struct action_t {
public:
  virtual void perform (app_state_t *s) = 0;
};

struct thread_exit_action_t {
public:
  thread_exit_action_t () {}
  void perform (app_state_t *s) { s->_n_alive --; } 
};

struct client_exit_action_t {
public:
  client_exit_action_t () {}
  void perform (app_state_t *s) { s->_client_alive = false; }
}; 

struct client_req_action_t {
public:
  client_req_action_t (req_t r) {}
  void perform (app_state_t *s) 
  {
    worker_t *w;
    if ((w = s->_ready.first)) {
      s->_ready.remove (w);
      cbv::ptr cb = r._done;
      r._done = mkevent (s->_rv, 
			 New refcounted<thread_done_action_t> (w, cb));
      mkevent (w->_rv, r)->signal ();
    } else {
      s->pending_

    }
  }

};

static void serve (int i, rendezvous_t<req_t> r)
{
  while (true) {
    req_t q;
    r.wait (q);
    if (q._time == 0) {
      return;
    } else {
      warn << i << ": serving request: " << q._time << "\n";
      sleep (q._time);
      warn << i << ": done serving.\n";
      r._done->signal ();
    }
  }
}

static void make_reqs (rendezvous_t<action_t, worker_t *, int> rv)
{
  for (int i = 0; i < 100; i++) {
    

  }
}

static void main2 ()
{


  for (size_t i = 0; i < workers.size (); i++) {
    workers[i]._index = i;

    cfork (G, 
	   New refcounted<thread_exit_action_t> (&n_alive), 
	   wrap (&workers[i], serve));

    _ready.insert_head (&workers[i]);
  }
  
  cfork (G, New refcounted<client_exit_action_t> (&client_alive), 
	 wrap (make_reqs, G));
  

  while (go) {
    action_t s;
    worker_t *w;
    int r;
    G.wait (s, w, r);
    switch (s) {
    case REQ_READY:
      if ((w = _ready.first)) {
	_ready.remove (w);
	mkevent (w->_rv, 
		 req_t (r, mkevent (G, THREAD_READY, w, 0)))->signal ();
      } else {
	pending.push_back (r);
      }
      break;
    case THREAD_READY: 
      _ready.insert_head (w);
      break;
    case CLIENT_DONE:
      warn << "Got shutdown signal from client.\n";
      go = false;
      break;
    case THREAD_EXIT:
      panic ("thread exitted unexpected\n");
      break;
    }
  }
  
  for (size_t i = 0; i < workers.size (); i++) {
    mkevent (workers[i]._rv, req_t (0, NULL))->signal ();
  }

  for (size_t i = 0; i < workers.size (); i++) {
    action_t s;
    worker_t *w;
    int r;
    G.wait (s, w, r);
    assert (s == THREAD_EXIT);
  }
  warn << "All done; shut down.\n";
  exit (0);
}


int main (int argc, char *argv[])
{
  main2 ();
  amain ();
}
