
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

  if (argc == 2) {
    ifn = argv[1];
    if (!(ifh = fopen (ifn.cstr (), "r"))) {
      warn << "cannot open file: " << ifn << "\n";
      usage ();
    }
    yyin = ifh;
  }

  int outfd;
  if (outfile) {
    if ((outfd = open (outfile.cstr (), O_CREAT|O_WRONLY)) < 0) {
      warn << "cannot open file for writing: " << outfile << "\n";
    }
  } else {
    outfd = 1;
  }

  yydebug = 1;
  
  // set up the dirty global variable to keep track of state while
  // running yacc
  // state = New parse_state_t ();

  yyparse ();

  if (ifh) {
    fclose (ifh);
  }

  state.output (outfd);
  close (outfd);

}
