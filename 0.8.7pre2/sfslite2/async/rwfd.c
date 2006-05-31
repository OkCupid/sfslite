/* $Id$ */

/*
 *
 * Copyright (C) 2000 David Mazieres (dm@uun.org)
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

#include "sysconf.h"

#if !defined (HAVE_ACCRIGHTS) && !defined (HAVE_CMSGHDR)
#error Do not know how to pass file descriptors
#endif /* !HAVE_ACCRIGHTS && !HAVE_CMSGHDR */

ssize_t
writevfd (int fd, const struct iovec *iov, int iovcnt, int wfd)
{
  struct msghdr mh;
#ifdef HAVE_CMSGHDR
  char cmhbuf[sizeof (struct cmsghdr) + sizeof (int)];
  struct cmsghdr *const cmh = (struct cmsghdr *) cmhbuf;
  int *const fdp = (int *) (cmhbuf + sizeof (struct cmsghdr));
#else /* !HAVE_CMSGHDR */
  int fdp[1];
#endif /* !HAVE_CMSGHDR */

  *fdp = wfd;
  bzero (&mh, sizeof mh);
  mh.msg_iov = (struct iovec *) iov;
  mh.msg_iovlen = iovcnt;

#ifdef HAVE_CMSGHDR
  mh.msg_control = (char *) cmh;
  mh.msg_controllen = cmh->cmsg_len = sizeof (cmhbuf) /* + sizeof (int) */;
  cmh->cmsg_level = SOL_SOCKET;
  cmh->cmsg_type = SCM_RIGHTS;
#else /* !HAVE_CMSGHDR */
  mh.msg_accrights = (char *) fdp;
  mh.msg_accrightslen = sizeof (fdp);
#endif /* !HAVE_CMSGHDR */

  return sendmsg (fd, &mh, 0);
}

ssize_t
writefd (int fd, const void *buf, size_t len, int wfd)
{
  struct iovec iov[1];
  iov->iov_base = (void *) buf;
  iov->iov_len = len;
  return writevfd (fd, iov, 1, wfd);
}


ssize_t
readvfd (int fd, const struct iovec *iov, int iovcnt, int *rfdp)
{
  struct msghdr mh;
#ifdef HAVE_CMSGHDR
  char cmhbuf[sizeof (struct cmsghdr) + sizeof (int)];
  struct cmsghdr *const cmh = (struct cmsghdr *) cmhbuf;
  int *const fdp = (int *) (cmhbuf + sizeof (struct cmsghdr));
#else /* !HAVE_CMSGHDR */
  int fdp[1];
#endif /* !HAVE_CMSGHDR */
  int n;

  *fdp = -1;
  bzero (&mh, sizeof mh);
  mh.msg_iov = (struct iovec *) iov;
  mh.msg_iovlen = iovcnt;

#ifdef HAVE_CMSGHDR
  mh.msg_control = (char *) cmh;
  mh.msg_controllen = cmh->cmsg_len = sizeof (cmhbuf) /* + sizeof (int) */;
  cmh->cmsg_level = SOL_SOCKET;
  cmh->cmsg_type = SCM_RIGHTS;
#else /* !HAVE_CMSGHDR */
  mh.msg_accrights = (char *) fdp;
  mh.msg_accrightslen = sizeof (fdp);
#endif /* !HAVE_CMSGHDR */

  n = recvmsg (fd, &mh, 0);
  *rfdp = *fdp;

  if (*fdp >= 0 && n == 0) {
    n = -1;
    errno = EAGAIN;
  }
  return n;
}

ssize_t
readfd (int fd, void *buf, size_t len, int *rfdp)
{
  struct iovec iov[1];
  iov->iov_base = buf;
  iov->iov_len = len;
  return readvfd (fd, iov, 1, rfdp);
}
