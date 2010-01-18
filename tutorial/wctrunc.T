// -*-c++-*-
#include "async.h"
#include "aios.h"
#include "tame.h"
#include "async.h"
#include "wide_str.h"
#include "parseopt.h"
#include <locale.h>

static void 
usage ()
{
  warn << "usage: wctrun <start> <len>\n";
  exit (1);
}

tamed static void
main_T (size_t start, size_t len)
{
  tvars {
    str s;
    bool go (true);
    int err;
  }

  while (go) {
    twait { ain->readline (mkevent (s, err)); }
    if (s) {
      wide_str_t ws (s);
      ws.chop (start, len);
      aout << "out: " << ws << "\n";
    } else {
      go = false;
    }
  }
  exit (0);
}


int 
main (int argc, char *argv[])
{
  // On linux use this:
  if (!wide_str_t::init ("en_US.UTF-8")) {
    warn << "Cannot initialize locale; quitting!\n";
    exit (-1);
  }
  size_t start, len;
  if (argc != 3 || 
      !convertint (argv[1], &start) ||
      !convertint (argv[2], &len)) {
    usage ();
  }
  main_T (start, len);
  amain ();
}
