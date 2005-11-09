
#include "unwrap.h"

static void
usage ()
{
  warnx << "usage: " << progname << " [-o <outfile>] <infile>\n";
  exit (1);
}

int
main (int argc, char *argv[])
{
  str ifn;
  setprogname (argv[0]);
  FILE *ifh;
  
  make_sync (0);
  make_sync (1);
  make_sync (2);

  if (argc < 2)
    usage ();

  ifn = argv[1];
  yydebug = 1;

  if (!(ifh = fopen (ifn.cstr (), "r"))) {
    warn << "cannot open file: " << ifn << "\n";
    usage ();
  }

  // set the input file accordingly
  yyin = ifh;
  
  yyparse ();
}
