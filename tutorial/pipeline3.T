// -*-c++-*-

#include "async.h"
#include "tame.h"
#include "tame_pipeline3.h"

using namespace pipeline3;

tamed static void
action (size_t i, evi_t ev)
{
  tvars {
    time_t d; 
  }
  d = rand () % 10;
  warn << "delay: " << i << " -> " << d << "\n";
  twait { delaycb (d, 0, mkevent ()); }
  ev->trigger (d);
}

tamed static void
main_T ()
{
  tvars {
    vec<int> slots;
    holdvar ptr<passive_control_t> c (passive_control_t::alloc (10, 0));
    runner_t r (c);
    size_t n (40);
    size_t i;
  }
  srand (time (NULL) ^ getpid ());

  slots.setsize (n);
  for (i = 0; i < n; i++) {
    twait { r.queue_for_takeoff (mkevent ()); }
    action (i, r.mkev (slots[i]));
  }
  twait { r.flush (mkevent ()); }
  for (i = 0; i < n; i++) {
    warn << "reported delay: " <<  i << " -> " << slots[i] << "\n";
  }
  exit (0);
}

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T ();
  amain ();
}

