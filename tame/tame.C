
/* $Id$ */

#include "tame.h"

parse_state_t state;

static void
usage ()
{
  warnx << "usage: " << progname << " [-Lhv] [-C <cceoc>] "
	<< "[-o <outfile>] [<infile>]\n"
	<< "\n"
	<< "  Flags:\n"
	<< "    -L  disable line number translation\n"
	<< "    -h  show this screen\n"
	<< "    -v  show version number and exit\n"
	<< "\n"
	<< "  Options:\n"
	<< "    -C  specify argname for checked call-exactly-once continuation"
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
  const char *c;
  str ifn;
  outputter_t *o;
  
  make_sync (0);
  make_sync (1);
  make_sync (2);

  while ((ch = getopt (argc, argv, "C:Lvdo:")) != -1)
    switch (ch) {
    case 'h':
      usage ();
      break;
    case 'C':
      cceoc_argname = optarg;
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

  if ((c = getenv ("CCEOC_ARGNAME")))
    cceoc_argname = c;

  argc -= optind;
  argv += optind;

  if (argc == 1) {
    ifn = argv[0];
    if (ifn != "-") {
      if (!(ifh = fopen (ifn.cstr (), "r"))) {
	warn << "cannot open file: " << ifn << "\n";
	usage ();
      }

      // the filename variable is local to scan.ll, which will need
      // it to output message messages. It defaults to '(stdin)'
      filename = ifn;

      yyin = ifh;
    }
  }

  state.set_infile_name (ifn);
  o = New outputter_t (ifn, outfile, (ifn && ifn != "-" && !no_line_numbers));
  if (!o->init ())
    exit (1);

  // only on if YYDEBUG is on :(
  //yydebug = 1;

  yyparse ();

  if (ifh) {
    fclose (ifh);
  }

  state.output (o);

  // calls close on the outputter fd
  delete o;
}
