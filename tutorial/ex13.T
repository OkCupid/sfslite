// -*-c++-*-
/* $Id: ex4.T 1749 2006-05-19 17:50:42Z max $ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "autocb.h"
#include "pipeline.h"

TAMED static void
do_rpc (ptr<aclnt> cli, bool *errp, size_t i, cbb cb)
{
  VARS {
    int res;
    clnt_stat err;
    bool ok (true);
    holdvar autocb_t<bool> acb (cb, ok);
  }
  BLOCK { cli->call (EX_RANDOM, NULL, &res, @(err)); }
  if (err) {
    warn << "RPC Failure in call " << i << ": " << err << "\n";
    *errp = true;
    ok = false;
  } else {
    warn << "Success(" << i << "): " << res << "\n";
  }
}

TAMED static void 
dostuff (str h, int port, cbb cb)
{
  // declare all of your "stack" variables here
  VARS {
    int fd (99999);
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
    bool err (false);
    int n_tot (40), window_sz (5);
  }


  // Call tcpconnect, and block until it returns; when it does return,
  // assign the local variable 'fd' to the result.
  BLOCK { tcpconnect (h, port, @(fd)); }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
    err = true;
  } else {
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);

    BLOCK { do_pipeline (window_sz, n_tot, 
			 wrap (do_rpc, cli, &err), @()); }

    warn << "All done...\n";
  }
  SIGNAL (cb, !err);
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
