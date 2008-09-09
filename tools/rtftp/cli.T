
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
  virtual void perform (evi_t ev) = 0;
  void connect (evb_t ev, CLOSURE);

  bool _verbose;
  str _file;
  str _host;
  int _port;

  ptr<axptr_stream> _x;
  ptr<aclnt> _cli;

};

//-----------------------------------------------------------------------

class put_cli_t : public cli_t {
public:
  put_cli_t (const str &f, const str &h, int p) : cli_t (f, h, p) {}
  void perform (evi_t ev) { perform_T (ev); }
private:
  void perform_T (evi_t ev, CLOSURE);
};

//-----------------------------------------------------------------------

class get_cli_t : public cli_t {
public:
  get_cli_t (const str &f, const str &h, int p, const str &d) 
    : cli_t (f, h, p), _dir (d) {}
  void perform (evi_t ev) { perform_T (ev); }
private:
  void perform_T (evi_t ev, CLOSURE);
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

// trigger: -2 on RPC failure
//          -1 on internal failure
//           0 on success
//           1 if no put due to file already existing
tamed void
put_cli_t::perform_T (evi_t ev)
{
  tvars {
    str dat;
    rtftp_file_t file;
    rtftp_status_t status;
    clnt_stat err;
    int rc (0);
  }

  if (!(dat = file2str (_file))) {
    warn ("cannot open file %s: %\m", _file.cstr ());
  } else {
    file.name = _file;
    file.data = dat;
    sha1_hash (file.hash.base (), file.data.base (), file.data.size ());
    twait { RPC::rtftp_program_1::rtftp_put (file, &status, mkevent (err)); }
    if (err) {
      warn  << "RPC failure: "  << err << "\n";
      rc = -2;
    } else if (status == RTFTP_EEXISTS) {
      if (_verbose) {
	warn << "file already exists\n";
      }
      rc = 1;
    } else if (put_res.status != RTFTP_OK) {
      warn << "cannot do PUT transfer: " << int (put_res.status);
      rc = -1;
    } else {
      rc = 0;
    }
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

static int
dir_ok (const char *d)
{
  int rc;
  struct stat sb;
  if (stat (d, &sb) == 0) {
    if (S_ISDIR (sb.st_mode)) rc = 1;
    else rc = -1;
  } else {
    rc = 0;
  }
  return rc;
}

//-----------------------------------------------------------------------

//
// write out the data 's' to the file given by _file.  Might need to make
// some parent directories if the filename contains slashes.
//
// return -1 if failure, and 0 if wrote the file out OK.
//
int
get_cli_t::write_file (const str &s)
{
  int rc = 0;
  if (chdir (_dir) != 0) {
    warn << "cannot chdir to " << _dir << "\n";
  } else {
    char *s = strdup (_file.cstr ());
    char *file = NULL;
    char *np;
    for (char *p = s; rc >= 0 && p && *p; np = p) {
      char *sl = strchr (p, '/');
      if (sl && *sl) {
	file = np = sl ++;
	*np = 0;
	int rc = dir_ok (p);
	if (rc < 0) {
	  warn ("cannot access dir %s from dir %s\n", p, getcwd ());
	} else if (mkdir (p) != 0) {
	  warn ("mkdir failed in %s on dir %s: %m\n", getcwd (), p);
	  rc = -1;
	}
	if (chdir (p) != 0) {
	  warn ("cannot chdir to %s from dir %s: %m\n", p, getcwd ());
	  rc = -1;
	}
      }
    }
    if (!file || *file) {
      warn ("no file to write!\n");
      rc = -1;
    } else if (!str2file (file, s, 0666, false)) {
      warn ("cannot create file %s in dir %s: %m\n", file, getcwd ());
      rc = -1;
    } else {
      rc = 0;
    }
  }
  return rc;
}

//-----------------------------------------------------------------------

// Trigger: -2 on RPC failure
//          -1 on internal failure
//           0 if file found and gotten
//           1 if file not found
tamed void
get_cli_t::perform_T (evi_t ev)
{
  tvars {
    rtftp_get_res_t res;
    rtftp_id_t arg;
    clnt_stat err;
    int rc (0);
  }

  arg = _file;
  twait { RPC::rtftp_program_1::rtftp_get (arg, &res, mkevent (err)); }
  if (err) {
    warn << "RPC failure: " << err << "\n";
    rc = -2;
  } else if (res.stauts == RTFTP_NOENT) {
    rc = 1;
  } else if (res.status != RTFTP_OK) {
    warn << "GET failed: " << int (res.status) << "\n";
    rc = -1;
  } else {
    if (!check_file (*res.file)) {
      warn << "hash mismatch on file!\n";
      rc = -1;
    } else {
      rc = write_file (res.file->data);
    }
  }
  ev->trigger (rc);
}

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
    twait { perform (mkevent (rc)); }
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
