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
  warn << "usage: utf8fix\n";
  exit (1);
}

tamed static void
main_T ()
{
  tvars {
    str s;
    bool go (true);
    int err;
  }

  while (go) {
    twait { ain->readline (mkevent (s, err)); }
    if (s) {
      s = utf8_fix (s, "\xef\xbf\xbd");
      aout << s << "\n";
    } else {
      go = false;
    }
  }
  exit (0);
}


int 
main (int argc, char *argv[])
{
  if (argc != 1) {
    usage ();
  }
  main_T ();
  amain ();
}
