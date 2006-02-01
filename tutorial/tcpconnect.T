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
TAME(static void connect_to_in_addr (in_addr a, int port, cbi::ptr cceoc))
{
  VARS {
    sockaddr_in sin;
    socklen_t sn;
    int err;
    int fd (-1);
  }
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

      BLOCK { fdcb (fd, selwrite, @()); }
      fdcb (fd, selwrite, NULL);

      sn = sizeof (sin);
      if (getpeername (fd, (sockaddr *) &sin, &sn)) {
	err = 0;
	sn = sizeof (err);
	getsockopt (fd, SOL_SOCKET, SO_ERROR, (char *) &err, &sn);
	if (err)
	  fd = -ECONNREFUSED;
      }
    } else {
      fd = -errno;
    }
  }
  RESUME (fd);
}

/**
 * A helper class that I'll eventually move into the tame.h library,
 * useful for canceling an TAME'd function mistream.
 */
class canceller_t {
public:
  canceller_t () {}

  // the cancelable function calls this 
  void wait (cbv b) { cb = b; }

  // the canceller calls this to cancel the cancelable function
  void cancel ()
  {
    if (cb) {
      cbv::ptr t = cb;
      cb = NULL;
      (*t) ();
    }
  }

  // the cancelable function can call this if it deems that it is too
  // late to cancel.
  void toolate () { cb = NULL; }
private:
  cbv::ptr cb;
};


/**
 * Given a hostname and port, make a TCP connection to the remote host,
 * returning the result as an opened file descriptor, or negative for
 * error.
 *
 * @return an object that can be used to cancel this computation mistream.
 */
TAME(static ptr<canceller_t> 
     my_tcpconnect (str hostname, u_int16_t port, cbi::ptr cceoc, 
		    bool dnssearch, str *namep))
{
  VARS {
    dnsreq_t *dnsp (NULL);
    ptr<hostent> h;
    int err;
    int fd (-1);
    ptr<canceller_t> canceller (New refcounted<canceller_t> ());
    join_group_t<bool> jg;
  }
  DEFAULT_RETURN { return canceller; }

  // launch 2 parallel calls that will race: the DNS requester
  // and the canceller.
  canceller->wait (@[jg,false]());
  dnsp = dns_hostbyname (hostname, @[jg,true](h, err), dnssearch); 

  JOIN (jg, completed) {
    if (!completed) {
      warn << "DNS lookup cancelled!\n";
      dnsreq_cancel (dnsp);
      jg.remove_join ();
      RESUME (fd);
    } 
  }

  // if we keep going, note that the canceller is still outstanding,
  // and might still cancel us.
  dnsp = NULL;
  if (!h) {
    canceller->toolate ();
    jg.remove_join ();
    if (dns_tmperr (err))
      UNBLOCK (-EAGAIN);
    else
      UNBLOCK (-ENOENT);
  } else {
    if (namep)
      *namep = h->h_name;
    
    connect_to_in_addr (*(in_addr *) h->h_addr, port, @[jg,true](fd)); 
    
    JOIN (jg, completed) {
      if (completed) {
	canceller->toolate ();
	jg.remove_join ();
      }
    }
    UNBLOCK (fd); 

    // wait on the call to connect_to_inaddr if we've timed out.
    if (jg.n_pending ()) 
      JOIN (jg, dummy) {}
  }
}

static void usage ()
{
  fatal << "usage: " << progname << " <hostname> <port>\n";
}

TAME (static void run (str s, int p))
{
  VARS {
    ptr<canceller_t> c;
    join_group_t<bool> jg;
    int fd;
  }
  c = my_tcpconnect (s, p, @[jg, true](fd), false, NULL);
  delaycb (4, 0, @[jg, false]());
  JOIN (jg, succeeded) {
    if (succeeded) {
      warn << "Succeeded; fd=" << fd << "\n";
    } else {
      warn << "Had to cancel!\n";
      c->cancel ();
    }
  }
  JOIN (jg, dummy) {}
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
