// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "ex_prot.h"

TAMED 
static 
void 
try_rpc (str h, int port, cbb cb)
{
  VARS {
    bool ret (false);
    int fd, r1;
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
 
    ex_str_t r2, a2;
    ex_struct_t r3;
    clnt_stat e1, e2, e3;
  }

  BLOCK { tcpconnect (h, port, @(fd)); }
 
  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
  } else {
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);
    a2 = "go hang a salami i'm a lasagna hog";
    
    BLOCK {
      cli->call (EX_RANDOM, NULL, &r1, @(e1));
      cli->call (EX_REVERSE, &a2, &r2, @(e2));
      cli->call (EX_STRUCT, NULL, &r3, @(e3));
    }
    if (e1 || e2 || e3) {
      warn << "at least 1 RPC failed!\n";
    } else {
      warn << "the results are in:\n"
	   << "\trandom # = " << r1 << "\n"
	   << "\treversed string = " << r2 << "\n"
	   << "\tstupid stuct = { s = " << r3.s << "; u = " << r3.u << " }\n";
      ret = true;
    }
  }
  cb->signal (true);
}
 
static void finish (bool rc)
{
  exit (rc ? 0 : -1);
}
 
int main (int argc, char *argv[])
{
  int port;
  if (argc != 3 || !convertint (argv[2], &port))
    fatal << "usage: ex2 <hostname> <port>\n";
  
  try_rpc (argv[1], port, wrap (finish));
  amain ();
}
