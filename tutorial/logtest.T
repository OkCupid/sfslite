// -*-c++-*-

#include "sfs_logger.h"
#include "parseopt.h"
#include "crypt.h"

//-----------------------------------------------------------------------

class test_t {
public:
  test_t () : _mode (0644), _go (true) {}
  bool config (int argc, char **argv);
  void run (evv_t ev, CLOSURE);

  void produce (sfs::logger_t *l, evv_t ev, CLOSURE);
  void consume (sfs::logger_t *l, evv_t ev, CLOSURE);
  void flush (sfs::logger_t *l, evv_t ev, CLOSURE);
  
  str _file;
  int _mode;
  vec<str> _q;
  bool _go;
};

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-m<mode>] <logfile>\n";
}

//-----------------------------------------------------------------------

static str
make_stuff (int i)
{
  strbuf b ("%d", i);
  str s = b;
  char hash[sha1::hashsize];
  sha1_hash (hash, s.cstr (), s.len ());
  str out = armor64 (hash, sha1::hashsize);
  strbuf b2;
  b2 << i << ": " << out << "\n";
  return b2;
}

//-----------------------------------------------------------------------

tamed void
test_t::produce (sfs::logger_t *l, evv_t ev)
{
  tvars {
    size_t i;
  }
  for (i = 0; i < 1000; i++) { 
    _q.push_back (make_stuff (i));
    twait { delaycb (0, 1000000, mkevent ()); }
  }
  _go = false;
  warn << "done producing..\n";
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
test_t::flush (sfs::logger_t *l, evv_t ev)
{
  tvars {
    bool ok;
  }
  while (_go) {
    twait { l->turn (mkevent (ok)); }
    if (!ok) {
      warn << "failed to turn log!!!\n";
    }
    twait { delaycb (1, 0, mkevent ()); }
  }
  warn << "done flushing...\n";
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
test_t::consume (sfs::logger_t *l, evv_t ev)
{
  tvars {
    vec<str> v;
    size_t i;
    bool ok;
  }
  while (_go || _q.size ()) {
    v.clear ();
    while (_q.size ()) {
      v.push_back (_q.pop_back ());
    }
    for (i = 0; i < v.size (); i++) {
      twait { l->log (v[i], mkevent (ok)); }
      if (!ok) {
	warn << "logging error encountered...\n";
      }
    }
    twait { delaycb (0, 100000, mkevent ()); }
  }
  warn << "done consuming...\n";
  ev->trigger ();
}

//-----------------------------------------------------------------------

bool
test_t::config (int argc, char **argv)
{
  int ch;
  bool ret = true;
  while ((ch = getopt (argc, argv, "m:")) != -1) {
    switch (ch) {
    case 'm': 
      if (!convertint (optarg, &_mode)) {
	warn << "cannot convert to Unix access mode: " << optarg << "\n";
	usage ();
	ret = false;
      }
      break;
    }
  }
  argc -= optind;
  argv += optind;
  if (argc != 1) {
    usage ();
    ret = false;
  } else {
    _file = argv[0];
  }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
test_t::run (evv_t ev)
{
  tvars {
    sfs::logger_t t (_self->_file, _self->_mode);
  }

  twait {
    produce (&t, mkevent ());
    consume (&t, mkevent ());
    flush (&t, mkevent ());
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed static void
main_T (int argc, char **argv)
{
  tvars {
    test_t t;
  }
  if (!t.config (argc, argv)) { exit (1); }
  twait { t.run (mkevent ()); }
  exit (0);
}

//-----------------------------------------------------------------------

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
