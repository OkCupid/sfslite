
/* $Id$ */

#include "unwrap.h"

parse_state_t state;

static void
usage ()
{
  warnx << "usage: " << progname << " [-L] [-o <outfile>] [<infile>]\n";
  exit (1);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  FILE *ifh = NULL;
  int ch;
  str outfile;
  bool debug = false;
  bool no_line_numbers = false;
  str ifn;
  
  make_sync (0);
  make_sync (1);
  make_sync (2);

  while ((ch = getopt (argc, argv, "Lvdo:")) != -1)
    switch (ch) {
    case 'L':
      no_line_numbers = true;
      break;
    case 'd':
      debug = true;
      break;
    case 'o':
      outfile = optarg;
      break;
    case 'v':
      warnx << "unwrap tool\n"
	    << "sfslite version " << VERSION << "\n";
      exit (0);
      break;
    default:
      usage ();
      break;
    }

  if (getenv ("NO_LINE_NUMBERS"))
    no_line_numbers = true;

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

  state.set_infile_name (ifn);
  state.set_xlate_line_numbers ((ifn && ifn != "-" && !no_line_numbers) 
				? true : false);

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
