// -*-c++-*-
/* $Id: tcpconnect.T 2693 2007-04-08 19:47:33Z max $ */

/*
 *
 * Copyright (C) 2003 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */
#include "async.h"
#include "dns.h"
#include "parseopt.h"
#include "tame.h"
#include "tame_io.h"
#include "tame_connectors.h"

static void usage ()
{
  fatal << "usage: " << progname << " <hostname> <port>\n";
}

#define BUFSZ 1024

tamed static void 
run (str s, int p, evv_t ev)
{
  tvars {
    int fd;
    char buf[BUFSZ];
    int rc;
    bool go (false);
  }
  while (true) {
    warn << "opening connection to " << s << ":" << p << "\n";
    twait { tcpconnect (s, p, mkevent (fd)); }
    if (fd >= 0) {
      warn << "connection succeeded\n";
      go = true;
      while (go) {
	twait { tame::waitread (fd, mkevent ()); }
	rc = read (fd, buf, BUFSZ);
	if (rc > 0) { }
	else if (rc == 0) { 
	  go = false; 
	  warn ("Hit EOF\n");
	} else if (errno != EAGAIN) { 
	  warn ("TCP connection error: %m\n");
	  go = false; 
	}
      }
      close (fd);
    } else {
      warn << "failed to connect!\n";
    }
    warn << "delay for 2 seconds\n";
    twait { delaycb (2, 0, mkevent ()); }
  }
  ev->trigger ();
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    int port;
  }
  if (argc != 3 || !convertint (argv[2], &port))
    usage ();
  twait { run (argv[1], port, mkevent ()); }
  exit (0);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  tcpconnect_debug = true;
  main2 (argc, argv);
  amain ();
}
