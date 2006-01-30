// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "arpc.h"

bool fix_bug;

/*
 * buggy1c
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

TAME(static void buggy1b (ceo_callback_bool_t cceoc))
{
  VARS { 
    join_group_t<> JG (__FILE__, __LINE__); 
    cb_hog_t *hog (New cb_hog_t ());
    int i;
  }

  NONBLOCK (JG) {
    delaycb (1, 0, @());
    hog->call_me (@());
  }

  for (i = 0; i < 1; i++) {
    JOIN (JG) {}
  }
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
