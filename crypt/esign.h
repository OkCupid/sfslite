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

#include "bigint.h"

class esign_pub {
public:
  bigint n;
  u_long k;

protected:
  bigint t;

  static void msg2bigint (bigint *resp, const str &msg, int bits);

public:
  esign_pub (const bigint &n, u_long k);
  bool raw_verify (const bigint &msg, const bigint &sig) const;
  bool verify (const str &msg, const bigint &sig) const {
    bigint z;
    msg2bigint (&z, msg, mpz_sizeinbase2 (&n));
    return raw_verify (z, sig);
  }
};

class esign_priv : public esign_pub {
public:
  bigint p;
  bigint q;

protected:
  bigint pq;

public:
  esign_priv (const bigint &p, const bigint &q, u_long k);
  bigint raw_sign (const bigint &m) const;
  bigint sign (const str &msg) const {
    bigint z;
    msg2bigint (&z, msg, mpz_sizeinbase2 (&n));
    return raw_sign (z);
  }
};

esign_priv esign_keygen (size_t nbits, u_long k = 4);
