// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
 
TAMED static 
void try_connect (str h, int port)
{
  VARS { int fd; }
  BLOCK { tcpconnect (h, port, @(fd)); }
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
