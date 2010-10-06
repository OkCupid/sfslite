// -*-c++-*-

#include "tame.h"
#include "tame_profiler.h"

tamed static void
D(int i, evv_t ev)
{
  twait { delaycb (i / 3, 0, mkevent ()); }
  ev->trigger ();
}

tamed static void
C(int i, evv_t ev)
{
  twait { delaycb (i / 2, 0, mkevent ()); }
  twait { D(i, mkevent ()); }
  ev->trigger ();
}

tamed static void
B(int i, evv_t ev)
{
  twait { C(i, mkevent ()); }
  if (i) {
    twait { delaycb (rand() % i, 0, mkevent ()); }
  }
  twait { D(i, mkevent()); }
  ev->trigger ();
}

tamed static void
A(int i, evv_t ev)
{
  tvars {
    int j;
  }
  twait { D(i, mkevent ()); }
  for (j = 0; j < i; j++) {
    twait { B(i, mkevent ()); }
  }
  twait { C(i, mkevent ()); }
  ev->trigger ();
}

tamed static void
run_loop ()
{
  tvars {
    int i;
  }
  twait {
    for (i = 0; i < 20; i++) {
      A(i, mkevent ());
    }
  }
  warn << "done...\n";
  twait { delaycb (1, 0, mkevent ()); }
  exit (0);
}

tamed static void
profile_loop ()
{
  tvars {
    tame::profiler_t *p (tame::profiler_t::profiler ());
  }
  p->enable ();
  while (true) {
    twait { delaycb (2, 0, mkevent ()); }
    p->report ();
  }
  p->disable ();
}

tamed static void
main_T ()
{
  profile_loop ();
  run_loop ();
}

int main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T ();
  amain ();
}
