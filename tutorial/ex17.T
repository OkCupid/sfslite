// -*-c++-*-
/* $Id: ex2.T 3476 2008-07-25 02:13:37Z max $ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "ex_prot.h"
#include "tame_rpc.h"
#include "tame_connectors.h"

class destructoid_t {
public:
  destructoid_t () : _id (id++) {}
  static int id;
  ~destructoid_t () { warn << "~" << _id << "\n"; }
  int _id;
};

int destructoid_t::id;

tamed static void
do_call (ptr<aclnt> c, evv_t ev)
{
  tvars {
    clnt_stat err (RPC_TIMEDOUT);
    int32_t out;
    holdvar destructoid_t d;
  }
  
  twait { 
    RPC::ex_prog_1::
      ex_random (c, &out, connector::klepto_timeout (mkevent (err), 1));
  }
  if (err) {
    warn << "RPC failure: " << err << "\n";
  } else {
    warn << "output: " << out << "\n";
  }
  ev->trigger ();

}

tamed static void 
try_rpc (str h, int port, evb_t cb)
{
  tvars {
    bool ret (false);
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
    size_t i;
    int fd;
  }

  twait { tcpconnect (h, port, mkevent(fd)); }
 
  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
  } else {
    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);

    for (i = 0; i < 20; i++) {
      twait { do_call (cli, mkevent ()); }
    }
    ret = true;
  }
  cb->trigger (ret);
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
