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

/* Fast-decryption variant of Paillier public key algorithm.
 *    Public-Key Cryptosystems Based on Composite Degree Residuosity Classes
 *    Pascal Paillier.  EUROCRYPT 1999.
 *
 * Two prime numbers p, q:
 *
 * Define N and k as:
 *    N = pq
 *    k = lcm[(p-1),(q-1)]
 *
 * The public key is (N).
 * The private key is (N, k).
 *
 * Let g = 2
 * Let a = random small prime (~160 bits) that divides k
 *
 * Define the following operations.  (Note D2 can only be
 * performed with the secret key.)  Let r be chosen uniformly at
 * random from [0,N) afresh each time E1 is called.
 *
 *  E (M)  =  g^M * g^{rN} % N^2
 *
 *  L(M)   =  M - 1 / N
 *  D2(M)  =  L (M^a % N^2)
 *  D(M)  =  [ D2(M)) / D2(g) ] % N
 *
 * Note that this fast variant replaces the following:
 * 
 *  Es(M)  =  g^M * r^N % N^2
 *  D2s(M) =  L (M^k % N^2)
 *
 * Public key operations on plaintext M < N, ciphertext C
 *   Encrypt:  E(M)
 *   Decrypt:  D(C)
 */

#include "crypt.h"
#include "prime.h"
#include "paillier.h"

INITFN (scrubinit);

static void
scrubinit ()
{
  mp_setscrub ();
}


paillier_pub::paillier_pub (const bigint &nn)
  : n (nn), 
    g (2), 
    nbits (max ((int) n.nbits (), 0)),
    fast (false)
{ 
  init ();
}

paillier_pub::paillier_pub (const bigint &nn, const bigint &gg)
  : n (nn), 
    g (gg), 
    nbits (max ((int) n.nbits (), 0)),
    fast (true)
{ 
  init ();
}

#include "bench.h"

void
paillier_pub::init ()
{
  nsq = n;
  mpz_square (&nsq, &n);

  if (fast)
    gn = powm (g, n, nsq);
}


bool
paillier_pub::E (bigint &m, const bigint &r) const
{
  if (m >= n) {
    warn ("paillier_pub::E: input too large [m %u, n %u]\n", 
	  m.nbits (), n.nbits ());
    return false;
  }
  
  bigint tmp;
  if (fast)
    // g^(nr) mod n^2
    tmp = powm (gn, r, nsq);
  else
    // r^n mod n^2
    tmp = powm (r, n, nsq);

  // g^m mod n^2
  m = powm (g, m, nsq);

  // r^n g^m mod n^2  OR  g^{m+nr} mod n^2
  m *= tmp;
  m %= nsq;

  return true;
}


/* Calculate CRT (mp,mq) mod N */
void
paillier_priv::CRT (bigint &mp, bigint &mq) const
{
#if _PAILLIER_CRT_

  // sp = mp * rp * q mod N
  mp *= rp;
  mp *= q;
  mp %= n;

  // sq = mq * rq * p mod N
  mq *= rq;
  mq *= p;
  mq %= n;

  // sp + sq
  mp += mq;

  if (mp >= n)
    mp -= n;

#endif
}


// Calculate fast decryption with chinese remainder and pre-computed values
void
paillier_priv::D (bigint &m, const bigint &msg) const
{
#if _PAILLIER_CRT_
  // mq = Lq (msg^a mod q^2) hq mod q
  bigint mq;
  if (fast)
    mq = powm (msg, a, qsq);
  else
    mq = powm (msg, q1, qsq);

  // Compute Lq (m)
  mq -= 1;
  mq *= lq;
  mq %= two_q;
  m  %= q;

  mq *= hq;
  mq %= q;

  // mp = Lp (msg^a mod p^2) hp mod p
  if (fast)
    m = powm (msg, a, psq);
  else 
    m = powm (msg, p1, psq);

  // Compute L_p(m)
  m -= 1;
  m *= lp;
  m %= two_p;
  m %= p;

  m *= hp;
  m %= p;

  // Recombine modulo residues
  CRT (m, mq);

#else /* PAILLIER_CRT */

  if (fast)
    m = powm (msg, a, nsq);
  else
    m = powm (msg, k, nsq);

  m -= 1;
  m *= ln;
  m %= two_n;
  m %= n;

  m *= hn;
  m %= n;

#endif
}


paillier_priv::paillier_priv (const bigint &pp, const bigint &qq, 
			      const bigint &aa, const bigint &gg, 
			      const bigint &kk, const bigint *nn) 
  : paillier_pub ((nn ? *nn : (pp * qq)), gg), p (pp), q (qq), a (aa), k (kk)
{
  assert (fast);
  init ();
}

paillier_priv::paillier_priv (const bigint &pp, const bigint &qq,
			      const bigint *nn)
  : paillier_pub ((nn ? *nn : (pp * qq))), p (pp), q (qq)
{
  init ();

  bigint p1 = p - 1;
  bigint q1 = q - 1;

  bigint kgcd;
  mpz_gcd (&kgcd, &p1, &q1);
  k  = p1 * q1;
  k /= kgcd;

}


void
paillier_priv::init ()
{
  assert (p < q);
  psq = p;
  mpz_square (&psq, &p);

  qsq = q;
  mpz_square (&qsq, &q);

  p1 = p - 1;
  q1 = q - 1;

  if (!fast) {
    bigint kgcd;
    mpz_gcd (&kgcd, &p1, &q1);
    k  = p1 * q1;
    k /= kgcd;
  }

#if _PAILLIER_CRT_
  rp = invert (q, p);
  rq = invert (p, q);

  two_p = pow (2, p.nbits ());
  two_q = pow (2, q.nbits ());

  lp = invert (p, two_p);
  lq = invert (q, two_q);

  if (fast) {
    hp  = powm (g, a, psq);
    hq  = powm (g, a, qsq);
  }
  else {
    hp  = powm (g, p1, psq);
    hq  = powm (g, q1, qsq);
  }
  hp -= 1;
  hp *= lp;
  hp %= two_p;
  hp  = invert (hp, p);

  hq -= 1;
  hq *= lq;
  hq %= two_q;
  hq  = invert (hq, q);

#else  /* PAILLIER_CRT */

  two_n = pow (2, n.nbits ());
  ln  = invert (n, two_n);

  if (fast) 
    hn  = powm (g, a, nsq);
  else
    hn  = powm (g, k, nsq);

  hn -= 1;
  hn *= ln;
  hn %= two_n;
  hn  = invert (hn, n);
#endif
}


static void 
paillier_gen (const bigint &p, const bigint &q, const bigint &n, 
	      const bigint &a, bigint &g, bigint &k)
{
  bigint p1 = p - 1;
  bigint q1 = q - 1;
  
  bigint kgcd;
  mpz_gcd (&kgcd, &p1, &q1);
  k  = p1 * q1;
  k /= kgcd;
    
  if (!p.probab_prime (5) || !q.probab_prime (5) || !a.probab_prime (5))
    fatal ("paillier_keygen: failed primality test\n");
  if ((k % a) != 0)
    fatal << "paillier_keygen: failed div test: " << (k % a) << "\n";
  
  g = powm (2, (k/a), n); 
}
		   

ptr<paillier_priv>
paillier_priv::make (const bigint &p, 
		     const bigint &q,
		     const bigint &a)
{
  if (p == q || p <= 1 || q <= 1
      || !p.probab_prime (5) || !q.probab_prime (5) || !a.probab_prime (5))
    return NULL;
  
  bigint g, k;
  bigint n = p * q;
  paillier_gen (p, q, n, a, g, k);

  return p < q 
    ? New refcounted<paillier_priv> (p, q, a, g, k, &n)
    : New refcounted<paillier_priv> (q, p, a, g, k, &n);
}


ptr<paillier_priv>
paillier_priv::make (const bigint &p, 
		     const bigint &q)
{
  if (p == q || p <= 1 || q <= 1
      || !p.probab_prime (5) || !q.probab_prime (5))
    return NULL;
  return p < q 
    ? New refcounted<paillier_priv> (p, q)
    : New refcounted<paillier_priv> (q, p);
}


paillier_priv
paillier_keygen (size_t bits, bool fast, u_int iter)
{
  random_init ();
  bigint p, q;

  if (!fast) {
    p = random_prime ((bits/2 + (bits & 1)), odd_sieve, 2, iter);
    q = random_prime ((bits/2 + 1),          odd_sieve, 2, iter);
    
    if (p > q)
      swap (p, q);

    return paillier_priv (p, q);
  }
  else {

    bigint a, g, k;

    a  = random_prime (paillier_priv::a_nbits, odd_sieve, 2, iter);
    
    size_t sbits = bits - (2 * paillier_priv::a_nbits);
    bigint cp = random_bigint (sbits/2 + (sbits & 1));
    bigint cq = random_bigint (sbits/2 + 1);
    
    p = a * cp + 1;
    while (!prime_test (p))
      // p1 = a * (++c1) + 1
      p += a;
    
    q = a * cq + 1;
    while (!prime_test (q))
      // p2 = a * (++c2) + 1
      q += a;
    
    bigint n = p * q;
    if (n.nbits () != bits && n.nbits () != (bits+1))
      return paillier_keygen (bits);

    paillier_gen (p, q, n, a, g, k);

    if (p > q)
      swap (p, q);
    
    return paillier_priv (p, q, a, g, k, &n);
  }
}


bigint
pre_paillier (const str &msg, size_t nbits)
{
  if (msg.len () > nbits) {
    warn ("pre_paillier: message too large [len %d]\n", msg.len ());
    return 0;
  }
  
  bigint r;
  mpz_set_rawmag_le (&r, msg.cstr (), msg.len ());
  return r;
}


str
post_paillier (const bigint &m, size_t msglen, size_t nbits)
{
  if (m.nbits () > nbits || msglen > nbits) {
    warn ("post_paillier: message too large [len %d buf %d bits %d]\n", 
	  m.nbits (), msglen, nbits);
    return NULL;
  }

  zcbuf msg (nbits);
  mpz_get_rawmag_le (msg, msg.size, &m);
  
  char *mp = msg;
  wmstr r (msglen);
  memcpy (r, mp, msglen);
  return r;
}
