// -*-c++-*-
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


#ifndef _SHA1_H_
#define _SHA1_H_

#include "crypthash.h"

class sha1 : public mdblock {
public:
  enum { hashsize = 20 };
  enum { hashwords = hashsize/4 };

  void finish () { mdblock::finish_be (); }

  static void newstate (u_int32_t state[hashwords]);
  static void transform (u_int32_t[hashwords], const u_char[blocksize]);
  static void state2bytes (void *, const u_int32_t[hashwords]);
};

class sha1ctx : public sha1 {
  u_int32_t state[hashwords];

  void consume (const u_char *p) { transform (state, p); }
public:
  sha1ctx () { newstate (state); }
  void reset () { count = 0; newstate (state); }
  void final (void *digest) {
    finish ();
    state2bytes (digest, state);
    bzero (state, sizeof (state));
  }
};

/* The following undefined function is to catch stupid errors */
extern void sha1_hash (u_char *digest, const iovec *, size_t);

inline void
sha1_hash (void *digest, const void *buf, size_t len)
{
  sha1ctx sc;
  sc.update (buf, len);
  sc.final (digest);
}

inline void
sha1_hashv (void *digest, const iovec *iov, u_int cnt)
{
  sha1ctx sc;
  sc.updatev (iov, cnt);
  sc.final (digest);
}

#ifdef _ARPC_XDRMISC_H_
template<class T> bool
sha1_hashxdr (void *digest, const T &t, bool scrub = false)
{
  xdrsuio x (XDR_ENCODE, scrub);
  XDR *xp = &x;
  if (!rpc_traverse (xp, const_cast<T &> (t)))
    return false;
  sha1_hashv (digest, x.iov (), x.iovcnt ());
  return true;
}
#endif /* _ARPC_XDRMISC_H_ */

class sha1oracle : public sha1 {
  const size_t hashused;
  const size_t nctx;
  u_int32_t (*state)[hashwords];
  bool firstblock;

  void consume (const u_char *p);
public:
  const u_int64_t idx;
  const size_t resultsize;

  sha1oracle (size_t nbytes, u_int64_t idx = 0, size_t hashused = hashsize);
  ~sha1oracle ();
  void reset ();
  void final (void *);
};

#endif /* !_SHA1_H_ */
