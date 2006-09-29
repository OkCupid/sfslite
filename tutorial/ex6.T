// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

tamed static 
void dostuff (vec<str> hosts, int port, cbi done)
{
  cvars {
    ptr<int> fd;
    u_int i;
    coordgroup_t<u_int, ptr<int> > CG (__FILE__, __LINE__);
  }

  for (i = 0; i < hosts.size (); i++) {
    fd = New refcounted<int> ();
    tcpconnect (hosts[i], port, mkevent (CG,i,fd,*fd) ); 
  }
  
  while (CG.need_join () && fd < 0) {
    cwait (CG, i, fd);
    warn << hosts[i]  << ":" << port << ": ";
    if (*fd > 0) {
      warnx << "connection succeeded\n";
    } else {
      warnx << "connection failed\n";
    }
  }
  (*done) (-1);
}

static void finish (int fd)
{
  delaycb (10, 0, wrap (exit, fd < 0 ? 1 : 0));
}

int
main (int argc, char *argv[])
{
  vec<str> hosts;
  int port;
  if (argc < 3 || !convertint (argv[1], &port))
    fatal << "usage: ex2 <port> <host1> <host2> ...\n";

  for (int i = 2; i < argc; i++) 
    hosts.push_back (argv[i]);
  
  dostuff (hosts, port, wrap (finish));
  amain ();
}
