/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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

#include "arpc.h"

inline void
axprt_stream::wrsync ()
{
  u_int64_t iovno = out->iovno () + out->iovcnt ();
  if (!syncpts.empty () && syncpts.back () == iovno)
    return;
  syncpts.push_back (iovno);
  out->breakiov ();
}

axprt_stream::axprt_stream (int f, size_t ps, size_t bs)
  : axprt (true, true), destroyed (false), ingetpkt (false), pktsize (ps),
    bufsize (bs ? bs : pktsize + 4), fd (f), cb (NULL), pktlen (0),
    wcbset (false)
{
  make_async (fd);
  close_on_exec (fd);
  out = New suio;
  pktbuf = NULL;
  bytes_sent = bytes_recv = 0;
}

axprt_stream::~axprt_stream ()
{
  destroyed = true;
  if (fd >= 0 && out->resid ())
    output ();
  fail ();
  delete out;
  xfree (pktbuf);
}

void
axprt_stream::setrcb (recvcb_t c)
{
  assert (!destroyed);
  cb = c;
  if (fd >= 0) {
    if (cb) {
      fdcb (fd, selread, wrap (this, &axprt_stream::input));
      if (pktlen)
	callgetpkt ();
    }
    else
      fdcb (fd, selread, NULL);
  }
  else if (cb)
    (*cb) (NULL, -1, NULL);
}

void
axprt_stream::setwcb (cbv c)
{
  assert (!destroyed);
  if (out->resid ())
    out->iovcb (c);
  else
    (*c) ();
}

void
axprt_stream::recvbreak ()
{
  warn ("axprt_stream::recvbreak: unanticipated break\n");
  fail ();
}

void
axprt_stream::sendbreak (cbv::ptr cb)
{
  static const u_int32_t zero[2] = {};
  suio_print (out, zero + 1, 4);
  if (cb)
    out->iovcb (cb);
  wrsync ();
  output ();
}

void
axprt_stream::fail ()
{
  if (fd >= 0) {
    fdcb (fd, selread, NULL);
    fdcb (fd, selwrite, NULL);
    wcbset = false;
    close (fd);
  }
  fd = -1;
  if (!destroyed) {
    ref<axprt> hold (mkref (this)); // Don't let this be freed under us
    if (cb)
      (*cb) (NULL, -1, NULL);
    out->clear ();
  }
}

int
axprt_stream::reclaim ()
{
  if (fd >= 0) {
    fdcb (fd, selread, NULL);
    fdcb (fd, selwrite, NULL);
    wcbset = false;
  }
  int rfd = fd;
  fd = -1;
  fail ();
  return rfd;
}

void
axprt_stream::sockcheck ()
{
  if (fd < 0)
    return;
  sockaddr_in sin;
  bzero (&sin, sizeof (sin));
  socklen_t sinlen = sizeof (sin);
  if (getsockname (fd, (sockaddr *) &sin, &sinlen) < 0)
    return;
  if (sin.sin_family == AF_INET) {
    for (in_addr *ap = ifchg_addrs.base (); ap < ifchg_addrs.lim (); ap++)
      if (*ap == sin.sin_addr)
	return;
    fail ();
  }
}

void
axprt_stream::sendv (const iovec *iov, int cnt, const sockaddr *)
{
  assert (!destroyed);
  u_int32_t len = iovsize (iov, cnt);

  if (fd < 0)
    panic ("axprt_stream::sendv: called after an EOF\n");

  if (len > pktsize) {
    warn ("axprt_stream::sendv: packet too large\n");
    fail ();
    return;
  }
  bytes_sent += len;
  len = htonl (0x80000000 | len);

  if (!out->resid () && cnt < min (16, UIO_MAXIOV)) {
    iovec *niov = New iovec[cnt+1];
    niov[0].iov_base = (iovbase_t) &len;
    niov[0].iov_len = 4;
    memcpy (niov + 1, iov, cnt * sizeof (iovec));

    ssize_t skip = writev (fd, niov, cnt + 1);
    if (skip < 0 && errno != EAGAIN) {
      fail ();
      return;
    }
    else
      out->copyv (niov, cnt + 1, max<ssize_t> (skip, 0));

    delete[] niov;
  }
  else {
    out->copy (&len, 4);
    out->copyv (iov, cnt, 0);
  }
  output ();
}

void
axprt_stream::output ()
{
  ssize_t n;
  int cnt;

  do {
    while (!syncpts.empty () && out->iovno () >= syncpts.front ())
      syncpts.pop_front ();
    cnt = syncpts.empty () ? (size_t) -1
      : int (syncpts.front () - out->iovno ());
  } while ((n = dowritev (cnt)) > 0);

  if (n < 0)
    fail ();
  else if (out->resid () && !wcbset) {
    wcbset = true;
    fdcb (fd, selwrite, wrap (this, &axprt_stream::output));
  }
  else if (!out->resid () && wcbset) {
    wcbset = false;
    fdcb (fd, selwrite, NULL);
  }
}

void
axprt_stream::ungetpkt (const void *pkt, size_t len)
{
  assert (len <= pktsize);
  assert (!pktlen);

  if (!pktbuf)
    pktbuf = (char *) xmalloc (bufsize);

  pktlen = len + 4;
  putint (pktbuf, 0x80000000|len);
  memcpy (pktbuf + 4, pkt, len);
  if (cb)
    callgetpkt ();
}

bool
axprt_stream::checklen (int32_t *lenp)
{
  int32_t len = *lenp;
  if (!(len & 0x80000000)) {
    // warn ("axprt_stream::checklen: invalid packet length: 0x%x\n", len);
    fail ();
    return false;
  }
  len &= 0x7fffffff;
  if ((u_int32_t) len > pktsize) {
    // warn ("axprt_stream::checklen: 0x%x byte packet is too large\n", len);
    fail ();
    return false;
  }
  *lenp = len;
  return true;
}

bool
axprt_stream::getpkt (char **cpp, char *eom)
{
  char *cp = *cpp;
  if (!cb || eom - cp < 4)
    return false;

  int32_t len = getint (cp);
  cp += 4;

  if (!len) {
    *cpp = cp;
    recvbreak ();
    return true;
  }
  if (!checklen (&len))
    return false;

  if ((eom - cp) < len)
    return false;
  *cpp = cp + len;
  (*cb) (cp, len, NULL);
  return true;
}

ssize_t
axprt_stream::doread (void *buf, size_t maxlen)
{
  return read (fd, pktbuf + pktlen, bufsize - pktlen);
}

void
axprt_stream::input ()
{
  if (fd < 0)
    return;

  ref<axprt> hold (mkref (this)); // Don't let this be freed under us

  if (!pktbuf)
    pktbuf = (char *) xmalloc (bufsize);

  ssize_t n = doread (pktbuf + pktlen, bufsize - pktlen);
  if (n <= 0) {
    if (n == 0 || errno != EAGAIN)
      fail ();
    return;
  }
  bytes_recv += n;
  pktlen += n;

  callgetpkt ();
}

void
axprt_stream::callgetpkt ()
{
  if (ingetpkt)
    return;
  ingetpkt = true;
  char *cp = pktbuf, *eom = pktbuf + pktlen;
  while (cb && getpkt (&cp, eom))
    ;
  if (cp != pktbuf)
    memmove (pktbuf, cp, eom - cp);
  pktlen -= cp - pktbuf;
  if (!pktlen) {
    xfree (pktbuf);
    pktbuf = NULL;
  }
  assert (pktlen < pktsize);
  ingetpkt = false;
}
