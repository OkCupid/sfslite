
// -*-c++-*-
/* $Id$ */

// unwrap.h needs to be included if you filter through unwrap...
#include "unwrap.h"

#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

UNWRAP_SD(void dostuff (str h, int port, cbb cb))
{
  // declare all of your "stack" variables here
  VARS {
    int fd;
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
    vec<int> results;
    vec<clnt_stat> errs;
    int n_tot (40), window_sz (5),  n_out (0), i (0);
    bool err (false);
  }

  CRCC { tcpconnect (h, port, @(fd)); }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
    err = true;
  } else {
    results.setsize (n_tot);
    errs.setsize (n_tot);
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);

    while (i < n_tot && !err) {
      CRCC* {
	while (n_out < window_sz && i < n_tot && !err) {
	  cli->call (EX_RANDOM, NULL, &results[i], @(i; (clnt_stat)$errs[@1]));
	  n_out++; i++;
	}
      } RESUME {
	if (errs[@1]) {
	  warn << "RPC error: " << errs[@1] << "\n";
	  err = true;
	} else {
	  warn << "Success " << @1 << ": " << results[@1] << "\n";
	}
	n_out --;
      }
    }
    
    // only want to continue past this point once
    if (n_out != 0)
      return;

    warn << "All done...\n";
  }
  (*cb) (!err);
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
