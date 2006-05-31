// -*-c++-*-
/* $Id$ */

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
#include "tame.h"
#include "parseopt.h"

/**
 * Given a UNIX in_addr structure and a port, make a new TCP connect
 * and test that it succeeded by selecting for write on the new socket.
 */
TAMED static ptr<canceller_t>
connect_to_in_addr (in_addr a, int port, coordvar_int_t coordvar)
{
  VARS {
    sockaddr_in sin;
    socklen_t sn;
    int err;
    int fd (-1);
    coordgroup_t<bool> cg;
    bool completed;
    ptr<canceller_t> ret (New refcounted<canceller_t> ());
  }
  DEFAULT_RETURN { return ret; }

  bzero (&sin, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons (port);
  sin.sin_addr = a;

  fd = inetsocket (SOCK_STREAM);
  if (fd >= 0) {
    make_async (fd);
    close_on_exec (fd);
    if (connect (fd, (sockaddr *) &sin, sizeof (sin)) >= 0 || 
	errno == EINPROGRESS) {

      ret->wait (@[cg, false]());
      fdcb (fd, selwrite, @[cg,true] ());
      fdcb (fd, selwrite, NULL);
      
      WAIT (cg, completed);
      warn << "return from function\n";

      if (completed) {
	sn = sizeof (sin);
	if (getpeername (fd, (sockaddr *) &sin, &sn)) {
	  err = 0;
	  sn = sizeof (err);
	  getsockopt (fd, SOL_SOCKET, SO_ERROR, (char *) &err, &sn);
	  if (err)
	    fd = -ECONNREFUSED;
	}
	ret->toolate ();
      }
      cg.remove_var ();
    } else {
      fd = -errno;
    }
  }

  // calls (*coordvar)(fd); also enforces that this
  // callback is called exactly once.
  SIGNAL (fd);
}

/**
 * Given a hostname and port, make a TCP connection to the remote host,
 * returning the result as an opened file descriptor, or negative for
 * error.
 *
 * @param hostname
 * @param port
 * @param coordvar The callback implicitly used in SIGNAL and RESUME statements
 * @param dnssearch whether to search DNS domains
 * @param namep the name of this host as gotten from DNS
 * @return an object that can be used to cancel this computation midstream.
 */
TAMED static ptr<canceller_t> 
my_tcpconnect (str hostname, u_int16_t port, coordvar_int_t coordvar,
	       bool dnssearch, str *namep)
{
  VARS {
    dnsreq_t *dnsp (NULL);
    ptr<hostent> h;
    int err;
    int fd (-1);
    ptr<canceller_t> canceller (New refcounted<canceller_t> ());
    ptr<canceller_t> cancel_connect;
    coordgroup_t<bool> cg (__FILE__, __LINE__);
    bool completed, dummy;
  }
  DEFAULT_RETURN { return canceller; }

  // launch 2 parallel calls that will race: the DNS requester
  // and the canceller.
  canceller->wait (@[cg,false]());
  dnsp = dns_hostbyname (hostname, @[cg,true](h, err), dnssearch); 

  WAIT (cg, completed);
  if (!completed) {
    warn << "DNS lookup cancelled!\n";
    dnsreq_cancel (dnsp);
    cg.remove_join ();
    RESUME (fd);
  } 

  // if we keep going, note that the canceller is still outstanding,
  // and might still cancel us.
  dnsp = NULL;
  if (!h) {
    canceller->toolate ();
    cg.remove_join ();
    if (dns_tmperr (err))
      SIGNAL (-EAGAIN);
    else
      SIGNAL (-ENOENT);
  } else {
    if (namep)
      *namep = h->h_name;
    
    cancel_connect = connect_to_in_addr (*(in_addr *) h->h_addr, 
					 port, @[cg,true](fd)); 

    WAIT (cg, completed);
    if (completed) {
      canceller->toolate ();
      cg.remove_join ();
    } else {
      cancel_connect->cancel ();
    }
    SIGNAL (fd); 
    
    // wait on the call to connect_to_inaddr if we've timed out.
    if (cg.n_vars_left ()) 
      WAIT (cg, dummy);
  }
}

static void usage ()
{
  fatal << "usage: " << progname << " <hostname> <port>\n";
}

TAMED static void 
run (str s, int p)
{
  VARS {
    ptr<canceller_t> c;
    coordgroup_t<bool> cg (__FILE__, __LINE__);
    int fd;
    bool succeeded, dummy;
  }
  c = my_tcpconnect (s, p, @[cg, true](fd), false, NULL);
  delaycb (4, 0, @[cg, false]());
  WAIT (cg, succeeded);
  if (succeeded) {
    warn << "Succeeded; fd=" << fd << "\n";
  } else {
    warn << "Had to cancel!\n";
    c->cancel ();
  }
  WAIT (cg, dummy);
  BLOCK { delaycb (20, 0, @()); }
  exit (0);
}


int
main (int argc, char *argv[])
{
  int port;
  setprogname (argv[0]);
  if (argc != 3 || !convertint (argv[2], &port))
    usage ();
  run (argv[1], port);
  amain ();
}
