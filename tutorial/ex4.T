// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

tamed static 
bool dostuff (const str &h, int port, cbb cb)
{
  // declare all of your "stack" variables here
  VARS {
    int fd (99999);
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
    vec<int> res;
    vec<clnt_stat> errs;
    int n_tot (40), window_sz (5), i;
    bool err_occurred (false);
    coordgroup_t<int> rpcback;
    bool ret (true);
    int return_id;
    int fds[2];
  }

  fds[0] = 1;
  fds[1] = 2;

  DEFAULT_RETURN { return ret; } 

  // Call tcpconnect, and block until it returns; when it does return,
  // assign the local variable 'fd' to the result.
  cwait { tcpconnect (h, port, mkevent(fd)); }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
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
      cli->call (EX_RANDOM, NULL, &res[i], mkevent (rpcback, i, errs[i]));
    }

    // a join is needed for every call that returns
    while (rpcback.n_joins_left ()) {

      // Block until a call has returned; when it has, get
      // the index variable associated with the call, and assign
      // it to 'return_id'.
      cwait (rpcback, return_id);
      if (errs[return_id]) {
	err_occurred = true;
	warn << "RPC error: " << errs[return_id] << "\n";
      } else {
	warn << "Success " << return_id << ": " << res[return_id] << "\n";
	
	// in the common case, for every call that returns,
	// another should be launched. but don't bother launching
	// more if an error occurred.
	if (i < n_tot && !err_occurred) {
	  cli->call (EX_RANDOM, NULL, &res[i], mkevent (rpcback, i, errs[i]));
	  i++;
	}
      }
    }
    warn << "All done...\n";
  }
  cb->signal (!err_occurred);
  return true;
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
  
  dostuff (argv[1], port, wrap (finish));
  amain ();
}
