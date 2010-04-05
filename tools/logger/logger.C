// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "arpc.h"
#include "aapp_prot.h"
#include "parseopt.h"

#define EC_ERR -2

//=======================================================================

class main_t {
public:
  main_t () : _fd (-1), _mode (0644) {}
  int config (int argc, char *argv[]);
  void dispatch (svccb *sbp);
  void turn (svccb *sbp);
  void log (svccb *sbp);
  void shutdown ();
  bool open ();
  void usage ();
  void close ();
  bool init ();
  bool run ();
  bool turn ();
private:
  int _fd;
  str _file;
  int _mode;
  ptr<axprt_unix> _x;
  ptr<asrv> _srv;
};

//=======================================================================

int 
main_t::config (int argc, char *argv[])
{
  int rc = 0;
  setprogname (argv[0]);
  int ch;

  while ((ch = getopt (argc, argv, "m:")) != -1) {
    switch (ch) {
    case 'm':
      if (!convertint (optarg, &_mode)) {
	warn << "bad file mode given: " << optarg << "\n";
	usage ();
	rc = EC_ERR;
	break;
      }
    default:
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1) {
    usage ();
    rc = EC_ERR;
  } else {
    _file = argv[0]; 
  }

  return rc;
}

//-----------------------------------------------------------------------

void
main_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    shutdown ();
  } else {
    switch (sbp->proc ()) {
    case LOGGER_LOG:
      log (sbp);
      break;
    case LOGGER_TURN:
      turn (sbp);
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
      break;
    }
  }
}

//-----------------------------------------------------------------------

void
main_t::turn (svccb *sbp)
{
  RPC::logger_prog_1::logger_turn_srv_t<svccb> srv (sbp);
  bool ret = turn ();
  srv.reply (ret);
}

//-----------------------------------------------------------------------

void
main_t::log (svccb *sbp)
{
  RPC::logger_prog_1::logger_log_srv_t<svccb> srv (sbp);
  const logline_t *arg = srv.getarg ();
  bool ret = false;
  if (_fd >= 0) {
    strbuf b;
    b << *arg;
    int rc = b.tosuio ()->output (_fd, -1);
    if (rc < 0) {
      warn ("write error in file %s: %m", _file.cstr ());
      turn ();
    } else {
      ret = true;
    }
  } 
  srv.reply (ret);
}

//-----------------------------------------------------------------------

void
main_t::shutdown ()
{
  close ();
  warn << "shutdown on EOF\n";
  exit (0);
}

//-----------------------------------------------------------------------

void
main_t::close ()
{
  if (_fd >= 0) {
    fsync (_fd);
    ::close (_fd);
    _fd = -1;
  }
}

//-----------------------------------------------------------------------

bool
main_t::turn ()
{
  close ();
  return open ();
}

//-----------------------------------------------------------------------

bool
main_t::init ()
{
  return open ();
}

//-----------------------------------------------------------------------

bool
main_t::run ()
{
  _x = axprt_unix::alloc (0);
  _srv = asrv::alloc (_x, logger_prog_1, wrap (this, &main_t::dispatch));
  if (!_srv) {
    warn << "Check that sfs_logger is launched from an sfslite application\n";
  }
  return _srv;
}

//-----------------------------------------------------------------------

bool
main_t::open ()
{
  bool ret = true;
  _fd = ::open (_file.cstr (), O_WRONLY | O_APPEND | O_CREAT, _mode);
  if (_fd < 0) {
    warn ("cannot open file '%s': %m\n", _file.cstr ());
    ret = false ;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
main_t::usage ()
{
  warnx << "usage: " << progname <<  " <logfile>\n";
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  main_t srv;
  int rc;
  setprogname (argv[0]);

  if ((rc = srv.config (argc, argv)) != 0) return rc;
  if (!srv.init ()) return EC_ERR;
  if (!srv.run ()) return EC_ERR;

  amain ();
  return 0;
}

//-----------------------------------------------------------------------
