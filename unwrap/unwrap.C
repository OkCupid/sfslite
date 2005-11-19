
/* $Id$ */

#include "unwrap.h"

parse_state_t state;

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

  if (argc == 1) {
    ifn = argv[0];
    if (ifn != "-") {
      if (!(ifh = fopen (ifn.cstr (), "r"))) {
	warn << "cannot open file: " << ifn << "\n";
	usage ();
      }
      yyin = ifh;
    }
  }

  int outfd;
  if (outfile && outfile != "-") {
    if ((outfd = open (outfile.cstr (), O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0) {
      warn << "cannot open file for writing: " << outfile << "\n";
    }
  } else {
    outfd = 1;
  }

  // only on if YYDEBUG is on :(
  //yydebug = 1;

  yyparse ();

  if (ifh) {
    fclose (ifh);
  }

  state.output (outfd);
  close (outfd);

}
