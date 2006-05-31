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

#include "amisc.h"

int
fdwait (int fd, bool r, bool w, timeval *tvp)
{
  static int nfd;
  static fd_set *fds;

  assert (fd >= 0);
  if (fd >= nfd) {
    // nfd = max (fd + 1, FD_SETSIZE);
    nfd = fd + 1;
    nfd = nfd + 0x3f & ~0x3f;
    xfree (fds);
    fds = (fd_set *) xmalloc (nfd >> 3);
    bzero (fds, nfd >> 3);
  }

  FD_SET (fd, fds);
  int res = select (fd + 1, r ? fds : NULL, w ? fds : NULL, NULL, tvp);
  FD_CLR (fd, fds);
  return res;
}

int
fdwait (int fd, selop op, timeval *tvp)
{
  switch (op) {
  case selread:
    return fdwait (fd, true, false, tvp);
  case selwrite:
    return fdwait (fd, false, true, tvp);
  default:
    panic ("fdwait: bad operation\n");
  }
}

