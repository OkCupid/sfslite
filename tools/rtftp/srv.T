
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "rtftp_prot.h"
#include "crypt.h"
#include "rxx.h"
#include "rtftp.h"

//-----------------------------------------------------------------------

class srv_t {
public:
  srv_t () 
    : _verbose (false), _debug (false), _port (RTFTP_TCP_PORT), _sfd (-1) {}
  int config (int argc, char *argv[]);
  void newcli ();
  bool init ();
  void run ();
private:
  void shutdown (int sig);
  bool _verbose, _debug;
  str _dir;
  int _port;
  int _sfd;
};

//-----------------------------------------------------------------------

class cli_t {
public:
  cli_t (int f, const char *a, bool v)
    : _fd (f), 
      _x (axprt_stream::alloc (f, MAX_PACKET_SIZE)),
      _srv (asrv::alloc (_x, rtftp_program_1, wrap (this, &cli_t::dispatch))),
      _addr (a),
      _verbose (v) {}
  void dispatch (svccb *sbp);
private:
  rtftp_status_t do_put (const rtftp_file_t *arg);
  void do_get (const rtftp_id_t &arg, rtftp_get_res_t *res);
  int _fd;
  ptr<axprt> _x;
  ptr<asrv> _srv;
  const str _addr;
  bool _verbose;
};

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-dv] [-p <port>] <dir>\n";
}

//-----------------------------------------------------------------------

rtftp_status_t
cli_t::do_put (const rtftp_file_t *arg)
{
  rtftp_status_t st;
  if (!check_file (*arg)) {
    st = RTFTP_CORRUPT;
  } else {
    str s = xdr2str (*arg);
    st = (write_file (arg->name, s) == 0) ? RTFTP_OK :  RTFTP_EFS;
  }
  if (_verbose) {
    warn << "PUT: " << arg->name << " -> ";
    rpc_print (warnx, st, 0, NULL, NULL);
    warnx << "\n";
  }
  return st;
}

//-----------------------------------------------------------------------

void
cli_t::do_get (const rtftp_id_t &arg, rtftp_get_res_t *res)
{
  str d = file2str (arg);
  res->set_status (RTFTP_OK);
  if (!d) {
    res->set_status (RTFTP_NOENT);
  } else if (!str2xdr (*res->file, d)) {
    warn << "cannot un-XDR file: " << arg << "\n";
    res->set_status (RTFTP_CORRUPT);
  } else if (!check_file (*res->file)) {
    warn << "checksum failed on read file: " << arg << "\n";
    res->set_status (RTFTP_CORRUPT);
  }
  if (_verbose) {
    warn << "GET: " << arg << " -> ";
    rpc_print (warnx, res->status, 0, NULL, NULL);
    warnx << "\n";
  }
}

//-----------------------------------------------------------------------

void
cli_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    _srv = NULL;
    _x = NULL;
    delete this;
    return;
  }

  switch (sbp->proc ()) {
  case RTFTP_NULL: 
    {
      RPC::rtftp_program_1::rtftp_null_srv_t<svccb> srv (sbp);
      srv.reply ();
      break;
    }

  case RTFTP_PUT:
    {
      RPC::rtftp_program_1::rtftp_put_srv_t<svccb> srv (sbp);
      rtftp_status_t s = do_put (srv.getarg ());
      srv.reply (s);
      break;
    }

  case RTFTP_GET:
    {
      rtftp_get_res_t res;
      RPC::rtftp_program_1::rtftp_get_srv_t<svccb> srv (sbp);
      do_get (*srv.getarg (), &res);
      srv.reply (res);
      break;
    }

  default: 
    {
      sbp->reject (PROC_UNAVAIL);
      break;
    }
  }
}

//-----------------------------------------------------------------------

void
srv_t::newcli ()
{
  sockaddr_in sin;
  socklen_t sinlen = sizeof (sockaddr_in);
  bzero (&sin, sinlen);
  
  int fd = accept (_sfd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
  if (fd < 0) {
    warn ("accept error: %m\n");
  } else {
    vNew cli_t (fd, inet_ntoa (sin.sin_addr), _verbose);
  }
}

//-----------------------------------------------------------------------

void
srv_t::shutdown (int sig)
{
  warn << "shutting down with signal=" << sig << "\n";
  exit (0);
}


//-----------------------------------------------------------------------

bool
srv_t::init ()
{
  bool rc = true;

  sigcb (SIGTERM, wrap (this, &srv_t::shutdown, SIGTERM));
  sigcb (SIGQUIT, wrap (this, &srv_t::shutdown, SIGQUIT));
  sigcb (SIGINT , wrap (this, &srv_t::shutdown, SIGINT));

  if (chdir (_dir.cstr ()) != 0) {
    warn ("cannot change to data directory %s: %m\n", _dir.cstr ());
    rc = false;
  } else {
    u_int32_t addr = INADDR_ANY;
    u_int16_t port = _port;
    int type = SOCK_STREAM;
    _sfd = inetsocket (type, port, addr);
    if (_sfd < 0) {
      warn ("cannot bind to port %d: %m\n", _port);
      rc = false;
    }
  }
  return rc;
}

//-----------------------------------------------------------------------

void
srv_t::run ()
{
  if (!_debug) {
    daemonize ("rtftpd");
  }
  warn << "starting up (pid=" << getpid () << ")\n";

  listen (_sfd, 200);
  fdcb (_sfd, selread, wrap (this, &srv_t::newcli));
}

//-----------------------------------------------------------------------

int
srv_t::config (int argc, char *argv[])
{
  int ch;
  int rc = 1;

  while ((ch = getopt (argc, argv, "vd")) != -1) {
    switch (ch) {
    case 'd':
      _debug = true;
      break;
    case 'v':
      _verbose = true;
      break;
    case 'p':
      if (!convertint (optarg, &_port)) {
	usage ();
	rc = -1;
	break;
      }
    case 'h':
      rc = 0;
      usage ();
      break;
    default:
      usage ();
      rc = -1;
      break;
    }
  }
  if (rc > 0) {
    argc -= optind;
    argv += optind;
    if (argc != 1) {
      usage ();
      rc = -1;
    } else {
      _dir = argv[0];
    }
  }
  return rc;
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  srv_t srv;
  int rc;
  setprogname (argv[0]);

  if ((rc = srv.config (argc, argv)) <= 0) {
    return rc;
  }

  if (!srv.init ()) {
    return -1;
  }

  srv.run ();
  amain ();
  return 0;
}

//-----------------------------------------------------------------------
