// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "arpc.h"

bool fix_bug;

/*
 * buggy1a
 *
 *   A test program that should complain about Bug #1a -- a closure
 *   leaked past the end of a function's control flow, from a function
 *   that only uses twait {...} (i.e., no NONtwait/JOIN).
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
  warn << "finish....\n";
  delaycb (2, 0, wrap (exit, rc ? 0 : -1));
}

tamed static 
void buggy1a (cbb cv)
{
  tvars {
    cb_hog_t *hog (New cb_hog_t ());
  }
  twait {
    delaycb (1, 0, mkevent()); 
    hog->call_me (mkevent());
  }
  TRIGGER (cv, true);
  twait { delaycb (1, 0, mkevent()); }

  if (fix_bug)
    delete hog;
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
  buggy1a (wrap (finish));
  amain ();
}
