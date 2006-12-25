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
#include "tame_io.h"

/**
 * Given a UNIX in_addr structure and a port, make a new TCP connect
 * and test that it succeeded by selecting for write on the new socket.
 */
tamed static ptr<canceller_t>
connect_to_in_addr (in_addr a, int port, cbi cv)
{
  tvars {
    sockaddr_in sin;
    socklen_t sn;
    int err;
    int fd (-1);
    rendezvous_t<bool> cg;
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

      ret->wait (mkevent(cg, false));
      waitwrite (fd, mkevent (cg, true));
      twait (cg, completed);
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
      } else {
	clearwrite (fd);
      }
    } else {
      fd = -errno;
    }
  }

  // calls (*callercv)(fd); also enforces that this
  // callback is called exactly once.
  TRIGGER(cv,fd);
}

/**
 * Given a hostname and port, make a TCP connection to the remote host,
 * returning the result as an opened file descriptor, or negative for
 * error.
 *
 * @param hostname
 * @param port
 * @param cv tha caller's CV
 * @param dnssearch whether to search DNS domains
 * @param namep the name of this host as gotten from DNS
 * @return an object that can be used to cancel this computation midstream.
 */
tamed static ptr<canceller_t> 
my_tcpconnect (str hostname, u_int16_t port, cbi cv,
	       bool dnssearch, str *namep)
{
  tvars {
    dnsreq_t *dnsp (NULL);
    ptr<hostent> h;
    int err;
    int fd (-1);
    ptr<canceller_t> canceller (New refcounted<canceller_t> ());
    ptr<canceller_t> cancel_connect;
    rendezvous_t<bool> cg (__FILE__, __LINE__);
    bool completed, dummy;
  }
  DEFAULT_RETURN { return canceller; }

  // launch 2 parallel calls that will race: the DNS requester
  // and the canceller.
  canceller->wait (mkevent (cg,false));
  dnsp = dns_hostbyname (hostname, mkevent (cg, true, h, err), dnssearch); 

  twait (cg, completed);
  if (!completed) {
    warn << "DNS lookup cancelled!\n";
    dnsreq_cancel (dnsp);
    TRIGGER(cv, fd);
  } else {
    // if we keep going, note that the canceller is still outstanding,
    // and might still cancel us.
    dnsp = NULL;
    if (!h) {
      canceller->toolate ();
      if (dns_tmperr (err))
	TRIGGER(cv, -EAGAIN);
      else
	TRIGGER(cv, -ENOENT);
    } else {
      if (namep)
	*namep = h->h_name;
      
      cancel_connect = connect_to_in_addr (*(in_addr *) h->h_addr, 
					   port, mkevent (cg,true,fd)); 
      twait (cg, completed);
      if (completed) {
	canceller->toolate ();
      } else {
	cancel_connect->cancel ();
      }
      TRIGGER(cv,fd);
      
      // wait on the call to connect_to_inaddr if we've timed out.
      if (cg.n_joins_left ()) 
	twait (cg, dummy);
    }
  }
}

static void usage ()
{
  fatal << "usage: " << progname << " <hostname> <port>\n";
}

tamed static void 
run (str s, int p)
{
  tvars {
    ptr<canceller_t> c;
    rendezvous_t<bool> cg (__FILE__, __LINE__);
    int fd;
    bool succeeded, dummy;
  }
  warn << "connect on " << s << ":" << p << "\n";
  c = my_tcpconnect (s, p, mkevent (cg, true, fd), false, NULL);
  delaycb (4, 0, mkevent (cg, false));
  twait (cg, succeeded);
  if (succeeded) {
    warn << "Succeeded; fd=" << fd << "\n";
  } else {
    warn << "Had to cancel!\n";
    c->cancel ();
  }
  twait (cg, dummy);
  twait { delaycb (10, 0, mkevent()); }
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
