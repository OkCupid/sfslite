// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

//
// ex5.T 
//
//   Functionally equivalent to ex4.T, but also tests support for
//   taming private class methods.
//
class remote_host_t {
public:
  remote_host_t (const str &n) : _name (n) {}
  void dostuff (int port, coordvar_t<bool>::t cb) 
  { dostuff_private (port, cb); }
private:
  void dostuff_private (int port, coordvar_t<bool>::t parentcv, CLOSURE);
  str _name;
};

// TAME_OFF
typedef enum { BLOCK_FOO = 1, NONBLOCK_FOO = 2, JOIN_FOO = 3, 
	       TAME_FOO = 5 } my_enum_t;
/* TAME_ON */

static void foo_func (my_enum_t e)
{
  /* TAME_OFF */
  switch (e) {
  case BLOCK_FOO:
    warn << "case 1\n";
    break;
  case NONBLOCK_FOO:
    warn << "case 2\n";
    break;
  case JOIN_FOO:
    warn << "case 3\n";
    break;
  case TAME_FOO:
    warn << "case 5\n";
    break;
  }
  // TAME_ON
}


TAMED void 
remote_host_t::dostuff_private (int port, coordvar_t<bool>::t parentcv)
{
  // declare all of your "stack" variables here
  VARS {
    int fd;
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
    vec<int> res;
    vec<clnt_stat> errs;
    int n_tot (40), window_sz (5), i;
    bool err_occurred (false);
    coordgroup_t<int> window;
    int return_id;
  }

  // Call tcpconnect, and block until it returns; when it does return,
  // assign the local variable 'fd' to the result.
  BLOCK { tcpconnect (_name, port, @(fd)); }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", _name.cstr(), port);
    err_occurred = true;
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

    // a join is needed for every call that returns
    while (window.n_vars_left ()) {

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
  }
  warn << "All done...\n";
  SIGNAL (!err_occurred);
}

static void finish (bool rc)
{
  exit (rc ? 0 : -1);
}

int
main (int argc, char *argv[])
{
  int port;
  if (argc != 3 || !convertint (argv[2], &port))
    fatal << "usage: ex2 <hostname> <port>\n";

  remote_host_t h (argv[1]);
  h.dostuff (port, wrap (finish));

  // TAME_OFF
  foo_func (TAME_FOO);
  // TAME_ON
  
  amain ();
}
