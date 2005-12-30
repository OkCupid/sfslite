// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "ex_prot.h"
#include "arpc.h"

/*
 * buggy1a
 *
 *   A test program that should complain about Bug #1a -- a closure
 *   leaked past the end of a function's control flow, from a function
 *   that only uses BLOCK {...} (i.e., no NONBLOCK/JOIN).
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
  delaycb (1, 0, wrap (exit, rc ? 0 : -1));
}

TAME(static void buggy1a (ceo_callback_bool_t cceoc))
{
  VARS {
    cb_hog_t *hog (New cb_hog_t ());
  }
  BLOCK {
    delaycb (1, 0, @()); 
    hog->call_me (@());
  }
  RESUME (true);
}
 
int main (int argc, char *argv[])
{
  buggy1a (wrap (finish));
  amain ();
}
