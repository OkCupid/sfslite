#if 0

#define USE_PCTR 0

#include "arpc.h"

int
main ()
{
  {
    xdrsuio x (XDR_ENCODE, true);
  }
  return 0;
}

#endif

#define USE_PCTR 0
#include "crypt.h"
#include "bench.h"

int
main (int argc, char **argv)
{
  random_update ();

#define HMAC(k, m)						\
do {								\
  u_char digest[sha1::hashsize];                                \
  sha1_hmac (digest, k, sizeof (k) - 1, m, sizeof (m) - 1);	\
  warn << "k = " << k << "\nm = " << m << "\n"			\
       << hexdump (digest, sizeof (digest)) << "\n";		\
} while (0)

#define HMAC2(k, k2, m)						\
do {								\
  u_char digest[sha1::hashsize];                                \
  sha1_hmac_2 (digest, k, sizeof (k) - 1, k2, sizeof (k2) - 1,	\
	       m, sizeof (m) - 1);				\
  warn << "k = " << k << "\nm = " << m << "\n"			\
       << hexdump (digest, sizeof (digest)) << "\n";		\
} while (0)

  HMAC ("Jefe", "what do ya want for nothing?");
  HMAC ("\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014", "Test With Truncation");
  //HMAC2 ("Je", "fe", "what do ya want for nothing?");

  rsa_priv x (rsa_keygen (1024));
  bigint pt (random_bigint (1019));
  bigint ct, pt2;
    
  BENCH (100000, ct = x.encrypt (pt));
  BENCH (1000, pt = x.decrypt (ct));
  //  BENCH (1000, pt = x.decrypt (ct, false)); /* No CRT */

#if 0
  warn << pt.getstr (10) << "\n";
  ct = x.encrypt (pt);
  warn << ct.getstr (10) << "\n";;
  pt2 = x.decrypt (ct);
  warn << pt2.getstr (10) << "\n";
#endif

  //  rabin_priv xx (rabin_keygen (1280, 2));
  rabin_priv xx (rabin_keygen (1024, 2));

  //  str pt3 ("plaintext message");
  str dt;

  BENCH (100000, ct = xx.encrypt (pt));
  BENCH (1000, pt2 = xx.decrypt (ct));

#if 0
  BENCH (100, ct = x.sign (pt3));
  BENCH (1000, x.verify (pt3, ct));
  BENCH (1000, ct = x.encrypt (pt3));
#endif

  return 0;
}
