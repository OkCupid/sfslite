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
  rabin_priv x (rabin_keygen (1280, 2));

  str pt ("plaintext message");
  bigint ct;
  str dt;

  BENCH (1000, ct = x.encrypt (pt));
  BENCH (100, dt = x.decrypt (ct, pt.len ()));

  BENCH (100, ct = x.sign (pt));
  BENCH (1000, x.verify (pt, ct));
  BENCH (1000, ct = x.encrypt (pt));
}
