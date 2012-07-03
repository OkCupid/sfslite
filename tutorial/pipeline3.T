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
    // initialize a pipeline controller which will execute up to 10 blocking
    // actions in parallel, and ensure there is a minimum of a 50 microsecond
    // delay between consecutive action launches
    // holdvar allows you to add a variable to a tvars block without using it
    // later in the function and not trigger a compile warning
    holdvar ptr<passive_control_t> c (passive_control_t::alloc (10, 50));
    runner_t r (c);
    size_t n (40);
    size_t i;
  }
  srand (time (NULL) ^ getpid ());

  slots.setsize (n);
  for (i = 0; i < n; i++) {
    // this returns immediately unless there are already 10 actions
    // outstanding, or it's been < 50 usec since the last launch
    // it'll block until an action finishes, or until the launch delay
    // time has passed
    twait { r.queue_for_takeoff (mkevent ()); }
    // this runs action and inserts its result into slots[i]
    action (i, r.mkev (slots[i]));
  }
  // harvest any outstanding actions
  twait { r.flush (mkevent ()); }
  // at this point we're guaranteed that all 40 actions have finished execution
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

