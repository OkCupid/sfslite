// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2005 Michael J. Freedman (mfreedman at alum.mit.edu)
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

#ifndef _PAILLIER_H_
#define _PAILLIER_H_ 1

// Use Chinese Remaindering for fast decryption
#define _PAILLIER_CRT_ 1

#include "crypt.h"
#include "bigint.h"

bigint pre_paillier (const str &msg, size_t nbits);
str post_paillier (const bigint &m, size_t msglen, size_t nbits);

class paillier_pub {
public:
  const bigint n;		/* Modulus */
  const bigint g;		/* Basis g */
  const int nbits;

protected:
  bool fast;
  bigint nsq;	   	        /* Modulus^2 */
  // _FAST_PAILLIER_  
  bigint gn;                    /* g^N % nsq */

  bool E (bigint &, const bigint &) const;
  void init ();

public:
  paillier_pub (const bigint &nn);
  paillier_pub (const bigint &nn, const bigint &gg);

  const bigint &ptxt_modulus () const { return n; }
  const bigint &ctxt_modulus () const { return nsq; }

  bigint encrypt (const str &msg) const {
    bigint m = pre_paillier (msg, nbits);
    if (!m)
      return 0;
    bigint r = random_bigint (nbits);
    r %= n;
    if (!E (m, r))
      return 0;
    return m;
  }

  bigint encrypt (const bigint &msg) const {
    bigint m = msg;
    bigint r = random_bigint (nbits);
    r %= n;
    if (!E (m, r))
      return 0;
    return m;
  }

  // Resulting ciphertext is sum of msgs' corresponding plaintexts
  // ctext = ctext1*ctext2 ==> ptext = ptext1 + ptext2
  bigint add (const bigint &msg1, const bigint &msg2) const {
    bigint m = msg1 * msg2;
    m %= nsq;
    return m;
  }

  // Resulting ciphertext is msg's corresponding plaintext * constant
  // ctext = ctext1^const ==> ptext = const * ptext1 
  bigint mult (const bigint &msg1, const bigint &const1) const {
    return powm (msg1, const1, nsq);
  }
};

class paillier_priv : public paillier_pub {
public:
  const bigint p;		/* Smaller prime */
  const bigint q;	        /* Larger prime  */
  const bigint a;               /* For fast decryption */
  enum { a_nbits = 160 };

protected:
  bigint p1;		        /* p-1             */
  bigint q1;		        /* q-1             */

  bigint k;	       	        /* lcm (p-1)(q-1)  */
  bigint psq;                   /* p^2             */
  bigint qsq;                   /* q^2             */

#if _PAILLIER_CRT_
  // Pre-computations
  bigint rp;                    /* q^{-1} % p */
  bigint rq;                    /* p^{-1} % q */

  bigint two_p;			/* 2^|p| */
  bigint two_q;			/* 2^|q| */

  bigint lp;			/* p^{-1} % 2^|p| */
  bigint lq;			/* q^{-1} % 2^|q| */

  bigint hp;			/* Lp (g^a % p^2) ^ {-1} % p */
  bigint hq;			/* Lq (g^a % q^2) ^ {-1} % q */
#else
  bigint two_n;                 /* 2^|n| */
  bigint ln;                    /* n^{-1} % 2^|n| */
  bigint hn;                    /* Ln (g^k % n^2) ^ {-1} % n */
#endif


  void init ();

  void CRT (bigint &, bigint &) const;
  void D   (bigint &, const bigint &) const;

public:
  paillier_priv (const bigint &pp, const bigint &qq, const bigint *nn = NULL);
  paillier_priv (const bigint &pp, const bigint &qq, 
		 const bigint &aa, const bigint &gg, 
		 const bigint &kk, const bigint *nn = NULL);

  // Use the slower version, yet more standard cryptographic assumptions
  static ptr<paillier_priv> make (const bigint &p, const bigint &q);
  // Use the fast decryption version
  static ptr<paillier_priv> make (const bigint &p, const bigint &q,
				  const bigint &a);

  str decrypt (const bigint &msg, size_t msglen) const {
    bigint m;
    D (m, msg);
    return post_paillier (m, msglen, nbits);
  }
};

paillier_priv paillier_keygen  (size_t nbits, bool fast = true, u_int iter = 32);

/*
 * Serialized format of a paillier private key:
 *
 * The private key itself is stured using the following XDR data structures:
 *
 * struct keyverf {
 *   asckeytype type;  // SK_PAILLIER_EKSBF
 *   bigint pubkey;    // The modulus of the public key
 * };
 *
 * struct privkey {
 *   bigint p;         // Smaller prime of secret key
 *   bigint q;         // Larger prime of secret key
 *   sfs_hash verf;    // SHA-1 hash of keyverf structure
 * };
 *
 * Option 1:  The secret key is stored without a passphrase
 *
 * "SK" SK_PAILLIER_EKSBF ",," privkey "," pubkey "," comment
 *
 *   SK_PAILLIER_EKSBF - the number 1 in ascii decimal
 *          privkey - a struct privkey, XDR and armor64 encoded
 *           pubkey - the public key modulus in hex starting "0x"
 *          comment - an arbitrary and possibly empty string
 *
 * Option 2:  There is a passphrase
 *
 * "SK" SK_PAILLIER_EKSBF "," rounds "$" salt "$" ptext "," seckey "," pubkey
 *   "," comment
 *
 *           rounds - the cost paraeter of eksblowfish in ascii decimal
 *             salt - a 16 byte random salt for eksblowfish, armor64 encoded
 *            ptext - arbitrary length string of the user's choice
 *        secretkey - A privkey struct XDR encoded, with 4 null-bytes
 *                    appended if neccesary to make the size a multiple
 *                    of 8 bytes, encrypted once with eksblowfish,
 *                    then armor64 encoded
 *       
 */

#if 0
enum asckeytype {
  SK_ERROR = 0,			// Keyfile corrupt
  SK_PAILLIER_EKSBF = 1,	// Paillier secret key encrypted with eksblowfish
};

const size_t SK_PAILLIER_SALTBITS = 1024;

inline str
file2wstr (str path)
{
  return str2wstr (file2str (path));
}
#endif

#endif /* !_PAILLIER_H_  */
