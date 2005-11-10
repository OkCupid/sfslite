
/* $Id$ */

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
  FILE *ifh = NULL;
  int ch;
  str outfile;
  bool debug = false;
  
  make_sync (0);
  make_sync (1);
  make_sync (2);

  while ((ch = getopt (argc, argv, "do:")) != -1)
    switch (ch) {
    case 'd':
      debug = true;
      break;
    case 'o':
      outfile = optarg;
      break;
    default:
      usage ();
      break;
    }

  argc -= optind;
  argv += optind;

  if (argc == 2) {
    ifn = argv[1];
    if (!(ifh = fopen (ifn.cstr (), "r"))) {
      warn << "cannot open file: " << ifn << "\n";
      usage ();
    }
    yyin = ifh;
  }

  if (debug) {
#ifdef YYDEBUG
    yydebug = 1;
#endif
  }
  
  yyparse ();
  if (ifh) {
    fclose (ifh);
  }
}
