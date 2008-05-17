// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "ex_prot.h"

tamed static void 
try_rpc (str h, int port, evb_t cb)
{
  tvars {
    bool ret (false);
    int32_t fd, r1;
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
 
    ex_str_t r2, a2;
    ex_struct_t r3;
    clnt_stat e1, e2, e3;
  }

  twait { tcpconnect (h, port, mkevent(fd)); }
 
  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
  } else {
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);
    a2 = "go hang a salami i'm a lasagna hog";
    
    twait {
      me::p_EX_PROG::v1::EX_RANDOM (cli, &r1, mkevent (e1));
      me::p_EX_PROG::v1::EX_REVERSE (cli, &a2, &r2, mkevent(e2));
      me::p_EX_PROG::v1::EX_STRUCT (cli, &r3, mkevent(e3));
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
  cb->trigger (true);
}
 
static void finish (bool rc)
{
  exit (rc ? 0 : -1);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool rc;
    int port;
  }
  if (argc != 3 || !convertint (argv[2], &port))
    fatal << "usage: ex2 <hostname> <port>\n";

  twait { try_rpc (argv[1], port, mkevent (rc)); }
  finish (rc);
}
 
int main (int argc, char *argv[])
{
  main2 (argc, argv);
  amain ();
  return 0;
}
