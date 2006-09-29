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
 *   that uses noncwaiting control...
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
part1 (cb_hog_t *hog, coordgroup_t<> *JG, cbv cb)
{
  delaycb (1, 0, mkevent (*JG));
  hog->call_me (mkevent (*JG));
  cb->signal ();
}

tamed static void part2 (coordgroup_t<> *JG, cbv cb)
{
  cvars { int i; }
  for (i = 0; i < 2; i++) {
    cwait (*JG);
  }
  cb->signal ();
}


tamed static void buggy1b (cbb cb)
{
  cvars { 
    coordgroup_t<> JG; 
    cb_hog_t *hog (New cb_hog_t ());
  }

  cwait { part1 (hog, &JG, mkevent()); }
  cwait { part2 (&JG, mkevent()); }

  if (fix_bug)
    delete hog;

  cb->signal (true);
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
