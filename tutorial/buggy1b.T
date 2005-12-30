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
 *   that uses NONBLOCK...
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

TAME(static void part1 (cb_hog_t *hog, join_group_t<> *JG, 
			ceo_callback_void_t cceoc))
{
  NONBLOCK (*JG) {
    delaycb (1, 0, @());
    hog->call_me (@());
  }
  RESUME;
}

TAME(static void part2 (join_group_t<> *JG, ceo_callback_void_t cceoc))
{
  VARS { int i; }
  for (i = 0; i < 2; i++) {
    JOIN (*JG) {}
  }
  RESUME;
}


TAME(static void buggy1b (ceo_callback_bool_t cceoc))
{
  VARS { 
    join_group_t<> JG; 
    cb_hog_t *hog (New cb_hog_t ());
  }

  BLOCK { part1 (hog, &JG, @()); }
  BLOCK { part2 (&JG, @()); }

  if (fix_bug)
    delete hog;

  RESUME (true);
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
