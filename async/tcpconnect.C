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

struct tcpconnect_t {
  u_int16_t port;
  cbi cb;
  int fd;
  dnsreq_t *dnsp;
  str *namep;

  tcpconnect_t (const in_addr &a, u_int16_t port, cbi cb);
  tcpconnect_t (str hostname, u_int16_t port, cbi cb,
		bool dnssearch, str *namep);
  ~tcpconnect_t ();

  void reply (int s) { if (s == fd) fd = -1; (*cb) (s); delete this; }
  void fail () { reply (-1); }
  void connect_to_name (str hostname, bool dnssearch);
  void name_cb (ptr<hostent> h, int err);
  void connect_to_in_addr (const in_addr &a);
  void connect_cb ();
};

tcpconnect_t::tcpconnect_t (const in_addr &a, u_int16_t p, cbi c)
  : port (p), cb (c), fd (-1), dnsp (NULL), namep (NULL)
{
  connect_to_in_addr (a);
}

tcpconnect_t::tcpconnect_t (str hostname, u_int16_t p, cbi c,
			    bool dnssearch, str *namep)
  : port (p), cb (c), fd (-1), dnsp (NULL), namep (NULL)
{
  connect_to_name (hostname, dnssearch);
}

tcpconnect_t::~tcpconnect_t ()
{
  if (dnsp)
    dnsreq_cancel (dnsp);
  if (fd >= 0)
    close (fd);
}

void
tcpconnect_t::connect_to_name (str hostname, bool dnssearch)
{
  dns_hostbyname (hostname, wrap (this, &tcpconnect_t::name_cb), dnssearch);
}

void
tcpconnect_t::name_cb (ptr<hostent> h, int err)
{
  dnsp = NULL;
  if (!h) {
    errno = ENOENT;
    fail ();
    return;
  }
  if (namep)
    *namep = h->h_name;
  connect_to_in_addr (*(in_addr *) h->h_addr);
}

void
tcpconnect_t::connect_to_in_addr (const in_addr &a)
{
  sockaddr_in sin;
  bzero (&sin, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons (port);
  sin.sin_addr = a;

  fd = inetsocket (SOCK_STREAM);
  if (fd < 0) {
    delaycb (0, wrap (this, &tcpconnect_t::fail));
    return;
  }
  make_async (fd);
  close_on_exec (fd);
  if (connect (fd, (sockaddr *) &sin, sizeof (sin)) < 0
      && errno != EINPROGRESS) {
    fail ();
    return;
  }
  fdcb (fd, selwrite, wrap (this, &tcpconnect_t::connect_cb));
}

void
tcpconnect_t::connect_cb ()
{
  fdcb (fd, selwrite, NULL);

  sockaddr_in sin;
  socklen_t sn = sizeof (sin);
  if (!getpeername (fd, (sockaddr *) &sin, &sn)) {
    reply (fd);
    return;
  }

  int err = 0;
  sn = sizeof (err);
  getsockopt (fd, SOL_SOCKET, SO_ERROR, (char *) &err, &sn);
  errno = err ? err : ECONNREFUSED;
  fail ();
}

tcpconnect_t *
tcpconnect (in_addr addr, u_int16_t port, cbi cb)
{
  return New tcpconnect_t (addr, port, cb);
}

tcpconnect_t *
tcpconnect (str hostname, u_int16_t port, cbi cb,
	    bool dnssearch, str *namep)
{
  return New tcpconnect_t (hostname, port, cb, dnssearch, namep);
}

void
tcpconnect_cancel (tcpconnect_t *tc)
{
  delete tc;
}
