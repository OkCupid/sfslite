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

  u_char digest[sha1::hashsize];
#define HMAC(k, m)						\
do {								\
  sha1_hmac (digest, k, sizeof (k) - 1, m, sizeof (m) - 1);	\
  warn << "k = " << k << "\nm = " << m << "\n"			\
       << hexdump (digest, sizeof (digest)) << "\n";		\
} while (0)

#define HMAC2(k, k2, m)						\
do {								\
  sha1_hmac_2 (digest, k, sizeof (k) - 1, k2, sizeof (k2) - 1,	\
	       m, sizeof (m) - 1);				\
  warn << "k = " << k << "\nm = " << m << "\n"			\
       << hexdump (digest, sizeof (digest)) << "\n";		\
} while (0)

  HMAC ("Jefe", "what do ya want for nothing?");
  HMAC ("\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014", "Test With Truncation");
  //HMAC2 ("Je", "fe", "what do ya want for nothing?");

#if 0
  rabin_priv x (rabin_keygen (1280, 2));

  str pt ("plaintext message");
  bigint ct;
  str dt;

  BENCH (1000, ct = x.encrypt (pt));
  BENCH (100, dt = x.decrypt (ct, pt.len ()));

  BENCH (100, ct = x.sign (pt));
  BENCH (1000, x.verify (pt, ct));
  BENCH (1000, ct = x.encrypt (pt));
#endif

  return 0;
}
