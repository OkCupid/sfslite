// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

struct state_t {
  state_t (str h, int p) : host (h), port (p), n_tot (40), window_sz (5) {}

  str host;
  int port;
  coordgroup_t<int> window;
  vec<int> res;
  vec<clnt_stat> errs;
  ptr<axprt_stream> x;
  ptr<aclnt> cli;
  int n_tot, window_sz;
  int i;

  void launch (coordvar_t<bool>::t coordvar, CLOSURE);
  void relaunch (coordvar_t<bool>::t coordvar, CLOSURE);
};

TAMED void 
state_t::launch (coordvar_t<bool>::t callercv)
{
  // declare all of your "stack" variables here
  VARS {
    int fd;
  }

  // Call tcpconnect, and block until it returns; when it does return,
  // assign the local variable 'fd' to the result.
  BLOCK { tcpconnect (host, port, @(fd)); }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", host.cstr(), port);
    RESUME (false);
  } else {
    res.setsize (n_tot);
    errs.setsize (n_tot);
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);

    for (i = 0; i < window_sz && i < n_tot; i++) {

      // launch an asychronous function call, but don't
      // block on its return. Associate the call with the 
      // join group 'window' and the index value 'i'
      NONBLOCK (window, i) {
	cli->call (EX_RANDOM, NULL, &res[i], @(errs[i]));
      }
    }
  }
  SIGNAL (true);
}

TAMED void 
state_t::relaunch (coordvar_t<bool>::t callercv)
{
  VARS {
    bool err_occurred (false);
    int return_id;
  }
  // a join is needed for every call that returns
  while (window.need_join ()) {
    
    // Block until a call has returned; when it has, get
    // the index variable associated with the call, and assign
    // it to 'return_id' for the scope of the JOIN block.
    WAIT (window, return_id);
    if (errs[return_id]) {
      err_occurred = true;
      warn << "RPC error: " << errs[return_id] << "\n";
    } else {
      warn << "Success " << return_id << ": " << res[return_id] << "\n";
      
      // in the common case, for every call that returns,
      // another should be launched. but don't bother launching
      // more if an error occurred.
      if (i < n_tot && !err_occurred) {
	NONBLOCK (window, i) {
	  cli->call (EX_RANDOM, NULL, &res[i], @(errs[i]));
	}
	i++;
      }
    }
  }
  warn << "All done...\n";
  SIGNAL (!err_occurred);
}

TAMED static 
void go (str h, int p)
{
  VARS {
    state_t s (h, p);
    bool rc;
  }
  BLOCK { s.launch (@(rc)); }
  if (rc) {
    BLOCK { s.relaunch (@(rc)); }
  }
  delaycb (0, 0, wrap (exit, rc ? 0 : -1));
}

int
main (int argc, char *argv[])
{
  int port;
  if (argc != 3 || !convertint (argv[2], &port))
    fatal << "usage: ex2 <hostname> <port>\n";

  go (argv[1], port);
  amain ();
}
