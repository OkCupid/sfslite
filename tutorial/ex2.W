// -*-c++-*-
/* $Id$ */

// unwrap.h needs to be included if you filter through unwrap...
#include "unwrap.h"

#include "ex2_prot.h"
#include "arpc.h"
#include "parseopt.h"

//
// A simple function that connects to the remote host:port pair,
// makes 3 parallel RPCs, dumps the results to stderr, and then 
// returns via the callback cb. Ignore the last arg...
//
static void dostuff (str h, int port, cbb cb, ptr<closure_t> c = NULL);

UNWRAP(void dostuff (str h, int port, cbb cb))
{
  // declare all of your "stack" variables here
  VARS {
    int fd;
    ptr<axprt_stream> x;
    ptr<aclnt> cli;

    int r1;
    ex2_str_t r2;
    ex2_struct_t r3;
    clnt_stat e1, e2, e3;

    ex2_str_t a2;

    bool ret;
    vec<int> rv;
    vec<clnt_stat> ev;
    int i;
    int n;
  }
  ret = false;

  // CRCC = "Call and Return to Current Continuation" 
  // Pass @(..) where the function expects a callback.  The arguments
  // to @(..) are which stack variables to stick the results into.
  CRCC {
    tcpconnect (h, port, @(fd));
  }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
  } else {
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex2_prog_1);

    a2 = "go hang a salami i'm a lasagna hog";

    // When given more than 1 call, only return to current continuation
    // when the last call returns.
    CRCC {
      cli->call (EX2_RANDOM, NULL, &r1, @(e1));
      cli->call (EX2_REVERSE, &a2, &r2, @(e2));
      cli->call (EX2_STRUCT, NULL, &r3, @(e3));
    }
    if (e1 || e2 || e3) {
      warn << "at least 1 RPC failed!\n";
    } else {
      warn << "the results are in:\n"
	   << "\trandom # = " << r1 << "\n"
	   << "\treversed string = " << r2 << "\n"
	   << "\tstupid stuct = { s = " << r3.s << "; u = " << r3.u << " }\n"
	;

      // in this next example, we dispatch a series of parallel RPCs in
      // a loop.  The only challenge here is that we need to stick the
      // values given to us by callbacks into array locations.  In 
      // practice, this involves wrapping an index variable (i below)
      // into the callback.
      n = 5;
      ev.setsize (n);
      rv.setsize (n);

      CRCC {

	for (i = 0; i < n; i++) {
	  //
	  // Now the callback expression, more complex, is given by:
	  //
	  //   @(i; (clnt_stat )$ev[@1] ) )
	  //
	  // The general format is @( args ; args ).  The args before
	  // the (optional) semicolon are values to be wrapped into the
	  // callback.  The values after the semicolon will accept
	  // the return values from the callback.  As seen below,
	  // the callback to aclnt::call takes a single parameter of
	  // type clnt_stat. We have to tell the unwrap tool the type
	  // of the variable (as seen by the C-cast).  The "$ev" means
	  // "stack variable ev"; finally, the @1 is a back reference to
	  // the first wrapped in argument, in this case i.
	  //
	  cli->call (EX2_RANDOM, NULL, &rv[i], @(i; (clnt_stat )$ev[@1] ) );
	}
      }
      ret = true;

      // check for n-fold success as usual
      for (i = 0 ; i < n; i++) {
	if (ev[i]) {
	  warn << "A failure: " << ev[i] << "\n";
	  ret = false;
	} else {
	  warn << "Result " << i << ": " << rv[i] << "\n";
	}
      }

    }
  }
  (*cb) (ret);
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
