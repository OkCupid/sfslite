
/* $Id$ */

#include "tame.h"

parse_state_t state;

static void
usage ()
{
  warnx << "usage: " << progname << " [-Lhv] [-o <outfile>] [<infile>]\n"
	<< "\n"
	<< "  Flags:\n"
	<< "    -L  disable line number translation\n"
	<< "    -h  show this screen\n"
	<< "    -v  show version number and exit\n"
	<< "\n"
	<< "  Options:\n"
	<< "    -o  specify output file\n"
	<< "\n"
	<< "  If no input or output files are specified, then standard in\n"
	<< "  and out are assumed, respectively.\n"
	<< "\n"
	<< "  Line number translation can also be suppressed by setting\n"
	<< "  the TAME_NO_LINE_NUMBERS environment variable.\n";
    
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
    case 'h':
      usage ();
      break;
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
      warnx << "tame\n"
	    << "sfslite version " << VERSION << "\n";
      exit (0);
      break;
    default:
      usage ();
      break;
    }

  if (getenv ("TAME_NO_LINE_NUMBERS"))
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
