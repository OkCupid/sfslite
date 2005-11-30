// -*-c++-*-
/* $Id$ */

#include "unwrap.h"
#include "parseopt.h"
 
UNWRAP_SD (void try_connect (str h, int port))
{
  VARS { int fd; }
  CRCC { tcpconnect (h, port, @(fd)); }
  if (fd >= 0) {
    warn << "Connection succeeded!\n";
    exit (0);
  } else {
    warn << "Connection failed!\n";
    exit (1);
  }
}
 
int main (int argc, char *argv[])
{
  int port;
  if (argc != 3 || !convertint (argv[2], &port))
    fatal << "usage: ex2 <hostname> <port>\n";
  
  try_connect (argv[1], port);
  amain ();
}
