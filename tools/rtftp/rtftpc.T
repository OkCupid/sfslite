
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "arpc.h"
#include "rtftp_prot.x"
#include "sha1.h"

//-----------------------------------------------------------------------

class cli_t {
public:
  cli_t (const str &s, const str &h, int p) 
    : _verbose (false), _file (f), _host (h), _port (p) {}

  virtual ~cli_t () {}
  void set_verbose (bool b) { _verbose = b; }
  void run (CLOSURE);

protected:
  virtual void perform (evb_t ev) = 0;
  void connect (evb_t ev, CLOSURE);

  bool _verbose;
  str _file;
  str _host;
  int _port;

  ptr<axptr_stream> _x;
  ptr<aclnt> _c;

};

//-----------------------------------------------------------------------

class put_cli_t : public cli_t {
public:
  put_cli_t (const str &f, const str &h, int p) : cli_t (f, h, p) {}
  void perform (evb_t ev) { perform_T (ev); }
private:
  void perform_T (evb_t ev, CLOSURE);
};

//-----------------------------------------------------------------------

class get_cli_t : public cli_t {
public:
  get_cli_t (const str &f, const str &h, int p, const str &d) 
    : cli_t (f, h, p), _dir (d) {}
  void perform (evb_t ev) { perform_T (ev); }
private:
  void perform_T (evb_t ev, CLOSURE);
  str _dir;
};

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-v] put <file> <host>\n"
	<< "            get <file> <host> <dir>\n";
}

//-----------------------------------------------------------------------

static bool
parse_host (const str &s, str *hp, int *pp) 
{
  static rxx x ("([^:]+)(:(\d+))?");
  bool ret = x.match (s);
  if (ret) {
    *hp = x[1];
    if (x[2]) {
      bool rc = convertint (x[3], pp);
      assert (rc);
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

static int
config (cli_t **clip, int argc, const char *const *argv)
{
  int rc = 1;
  bool verbose = false;
  int port = RTFTP_TCP_PORT;
  str host;
  str file;
  str dir;
  cli_t *cli = NULL;
  
  while ((ch = getopt (argc, argv, "vh"))) {
    switch (ch) {
    case 'v':
      verbse = true;
      break;
    case 'h':
      usage ();
      rc = 0;
      break;
    default:
      usage ();
      rc = -1;
    }
  }

  argc -= optind;
  argv += optind;

  if (rc > 0) {
    bool err = false;
    if (argc < 3 || argc > 4) {
      err = true;
    } else if (!parse_host (argv[2], &host, &port)) {
      warn << "bad hostname/port: " << argv[2];
      err = true;
    } else if (strcmp (argv[0], "put") == 0) {
      if (argc != 3) {
	err = true;
      } else {
	file = argv[1];
      }
      cli = New put_cli_t (file, host, port);
    } else if (strcmp (argv[0], "get") == 0) {
      mode = GET;
      if (argc != 4) {
	err = true;
      } else {
	file = argv[1];
	dir = argv[3];
      }
      cli = New get_cli_t (file, host, port, dir);
    } else {
      warn << "need either put/get operation mode\n";
      err = true;
    }
   
    if (err) {
      usage ();
      rc = -1;
    } else {
      cli->set_verbose (verbose);
    }
  }
  *clip = cli;
  return rc;
}

//-----------------------------------------------------------------------

tamed void
cli_t::connect (evb_t ev)
{
  tvars {
    int fd;
    bool ret (true);
  }
  twait { tcpconnect (_host, _port, mkevent (fd)); }
  if (fd < 0) {
    warn << "connection to " << _host << ":" << _port << " failed\n";
    ret = false;
  } else {
    _x = axprt_stream::alloc (fd, MAX_PACKET_SIZE);
    _cli = aclnt::alloc (_x, rtftp_program_1);
  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
put_cli_t::perform_T (evb_t ev)
{
  tvars {
    int fd;
    rendezvous_t<> rv;
    int rc;
    rtftp_chnk_t arg (RTFTP_OK);
    u_int32_t chnk_id (0);
    u_int64_t xfer_id;
    rtftp_file_t file_id;
    rtftp_put_res_t put_res;
    bool go (true);
    clnt_stat err;
    int wsize (10);
    sha1ctx sha;
    bool eof (false);
    int nsent (0), nrecv (0);
  }

  fd = open (_file.cstr (), O_RDONLY);
  if (fd < 0) {
    warn ("cannot open file %s: %\m", _file.cstr ());
  } else {

    file_id = _file;

    twait { 
      RPC::rtftp_program_1::rtftp_put (file_id, &st, &put_res, mkevent (err)); 
    }

    if (err) {
      warn << "RPC failure in put: " << err << "\n";
    } else if (put_res.status == RTFTP_EEXISTS) {
      if (_verbose) {
	warn << "file already exists\n";
      }
      ok = true;
    } else if (put_res.status != RTFTP_OK) {
      warn << "cannot initiate PUT transfer: " << int (put_res.status);
    } else {
      xfer_id = *put_res.xfer_id;
      while (go) {

	arg.dat->xfer_id = xfer_id;
	arg.chnk_id = chnk_id++;
	
	arg.dat->dat.set_size (CHNKSZ);
	rc = read (fd, arg.dat->dat.base (), CHNKSZ);
	if (rc > 0) {
	  arg->dat->dat.set_size (rc);
	  sha.update (arg.dat->dat.base (), rc);
	} else if (rc == 0) {
	  eof = true;
	  arg->set_status (RTFTP_EOF);
	  arg->footer
	}
      }
    }
  }
}

#undef BUFSZ

//-----------------------------------------------------------------------

tamed void
cli_t::run ()
{
  tvars {
    bool ok;
    int rc (0);
  }

  twait { connect (mkevent (ok)); }
  if (ok) {
    twait { perform (mkevent (ok)); }
    if (!ok) {
      warn << "operation failed\n";
      rc = -1;
    }
  } else {
    warn << "connection failed: " << _host << ":" << _port << "\n";
    rc = -1;
  }

  exit (rc);
}

//-----------------------------------------------------------------------

int 
main (int argc, char *argv[])
{
  cli_t *cli;
  int rc;
  setprogname (argv[0]);

  if ((rc = config (&cli, argc, argv)) <= 0) {
    return rc;
  }

  cli->run ();
  amain ();

  return 0; // never get here
}
