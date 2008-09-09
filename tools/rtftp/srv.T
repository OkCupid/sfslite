
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "rtftp_prot.h"
#include "sha1.h"
#include "rxx.h"
#include "rtftp.h"

//-----------------------------------------------------------------------

class srv_t {
public:
  srv_t () : _verbose (false), _port (RTFTP_TCP_PORT), _sfd (-1) {}
  int config (int argc, char *argv[]);
  void newcli ();
  bool run ();
private:
  bool _verbose;
  str _dir;
  int _port;
  int _sfd;
};

//-----------------------------------------------------------------------

class cli_t {
public:
  cli_t (int f, const char *a)
    : _fd (f), 
      _x (axprt_stream::alloc (f, MAX_PACKET_SIZE)),
      _srv (asrv::alloc (_x, rtftp_program_1, wrap (this, &cli_t::dispatch))),
      _addr (a) {}
  void dispatch (svccb *sbp);
private:
  void do_put (const rtftp_file_t *arg, rtftp_status_t *res);
  void do_get (const rtftp_id_t *arg, rtftp_get_res_t *res);
  int _fd;
  ptr<axprt> _x;
  ptr<asrv> _srv;
  const str _addr;
};

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-p <port>] <dir>\n";
}

//-----------------------------------------------------------------------

void
cli_t::do_put (const rtftp_file_t *arg, rtftp_status_t *res)
{

}

//-----------------------------------------------------------------------

void
cli_t::do_get (const rtftp_id_t *arg, rtftp_get_res_t *res)
{

}

//-----------------------------------------------------------------------

void
cli_t::dispatch (svccb *sbp)
{
  if (!sbp) {
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
      rtftp_status_t s;
      RPC::rtftp_program_1::rtftp_put_srv_t<svccb> srv (sbp);
      do_put (srv.getarg (), &s);
      srv.reply (s);
      break;
    }

  case RTFTP_GET:
    {
      rtftp_get_res_t res;
      RPC::rtftp_program_1::rtftp_get_srv_t<svccb> srv (sbp);
      do_get (srv.getarg (), &res);
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
    vNew cli_t (fd, inet_ntoa (sin.sin_addr));
  }
}

//-----------------------------------------------------------------------

bool
srv_t::run ()
{
  bool rc = true;
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
    } else {
      listen (_sfd, 200);
      fdcb (_sfd, selread, wrap (this, &srv_t::newcli));
    }
  }
  return rc;
}

//-----------------------------------------------------------------------

int
srv_t::config (int argc, char *argv[])
{
  int ch;
  int rc = 1;

  while ((ch = getopt (argc, argv, "v")) != -1) {
    switch (ch) {
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

  srv.run ();
  amain ();
  return 0;
}

//-----------------------------------------------------------------------
