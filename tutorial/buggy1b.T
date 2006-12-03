// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "arpc.h"

bool fix_bug;

/*
 * buggy1b
 *
 *   A test program that should complain about Bug #1b -- a closure
 *   leaked past the end of a function's control flow, from a function
 *   that uses nontwaiting control...
 */

class cb_hog_t {
public:
  cb_hog_t () {}
  void call_me (cbv c) { _cb = c;  (*c)();  }
private:
  cbv::ptr _cb;
};
 
static void finish (bool rc)
{
  delaycb (2, 0, wrap (exit, rc ? 0 : -1));
}

tamed static void 
part1 (cb_hog_t *hog, rendezvous_t<> *rv, cbv cb)
{
  delaycb (1, 0, mkevent (*rv));
  hog->call_me (mkevent (*rv));
  TRIGGER (cb);
}

tamed static void part2 (rendezvous_t<> *rv, cbv cb)
{
  tvars { int i; }
  for (i = 0; i < 2; i++) {
    twait (*rv);
  }
  TRIGGER (cb);
}


tamed static void buggy1b (cbb cb)
{
  tvars { 
    rendezvous_t<> rv; 
    cb_hog_t *hog (New cb_hog_t ());
  }

  twait { part1 (hog, &rv, mkevent()); }
  twait { part2 (&rv, mkevent()); }

  if (fix_bug)
    delete hog;

  TRIGGER (cb, true);
}

static void usage ()
{
  fatal << "usage: " << progname << " [-f]\n";
}
 
int main (int argc, char *argv[])
{
  int ch;
  fix_bug = false;
  setprogname (argv[0]);
  while ((ch = getopt (argc, argv, "f")) != -1) {
    switch (ch) {
    case 'f':
      fix_bug = true;
      break;
    default:
      usage ();
    }
  }
  buggy1b (wrap (finish));
  amain ();
}
