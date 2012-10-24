// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "parseopt.h"
#include "tame.h"

//=======================================================================

struct app_t {
  static app_t *init (int argc, char *argv[]);
  app_t (int port, size_t b) : _port (port), _buf_sz (b) {}
  static void usage ();
  virtual void run (CLOSURE) = 0;
  static size_t _def_bufsz;
protected:
  int _port;
  size_t _buf_sz;
};

//=======================================================================

class client_t : public app_t {
public:
  client_t (str h, int p, size_t b) : app_t (p, b), _host (h) {}
  void run (CLOSURE) {}
protected:
  str _host;
};

//=======================================================================

class server_t : public app_t {
public:
  server_t (int p, str file, size_t b) : app_t (p, b), _file (file) {}
  void run (CLOSURE) {}
protected:
  str _file;
};

//=======================================================================

size_t app_t::_def_bufsz = 0x100000;

//-----------------------------------------------------------------------

app_t *
app_t::init (int argc, char *argv[])
{
  int ch;
  str file;
  bool err = false;
  int port = -1;
  str hostname;
  size_t bufsz = _def_bufsz;

  while ((ch = getopt (argc, argv, "p:")) != -1) {
    switch (ch) {
    case 'p':
      if (!convertint (optarg, &port)) {
	warn << "cannot convert '" << optarg << "' to a port\n";
	err = true;
      }
      break;
    case 'f':
      file = optarg;
      break;
    case 'h':
      err = true;
      break;
    case 'b':
      if (!convertint (optarg, &bufsz)) {
	warn << "cannot convert '" << optarg << "' to a buffer size\n";
	err = true;
      }
      break;
    default:
      break;
    }
  }
  argv += optind;
  argc -= optind;

  if (argc == 1) {
    hostname = argv[0];
  } else if (argc != 0) {
    warn << "unexpected arguments received\n";
    err = true;
  }

  if (port < 0) {
    warn << "Expected a port specified with -p\n";
    err = true;
  }

  if (hostname && file) {
    warn << "Cannot use the -f flag in client mode\n";
    err = true;
  }

  if (err) {
    usage ();
  } else if (hostname) {
    vNew client_t (hostname, port, bufsz);
  } else {
    vNew server_t (port, file, bufsz);
  }

  return NULL;
}

//-----------------------------------------------------------------------

void
app_t::usage ()
{
  warnx << "usage: " << progname << " [-f <file>] [-b <bufsz>]"
	<< " -p <port> [<hostname>]\n"
	<< "\n"
	<< " nc++ (named ncpp for automake reasons) is a netcat improvement\n"
	<< " that does nice things like allowing multiple clients to connect\n"
	<< " to the same port or disconnecting clients aggressively on\n"
	<< " failure.\n"
	<< "\n"
	<< "  - run as a server without the hostname argument\n"
	<< "  - run as a client with the hostname arugment\n"
	<< "  - specify -p for port in both cases\n"
	<< "\n"
	<< "Options:\n"
	<< "  -f <file>    output to a file in server mode\n"
	<< "  -b <bufsz>   tune the buffer size (default is " 
	<< _def_bufsz <<  ")\n"
	<< "\n";
}

//=======================================================================

int main(int argc, char *argv[])
{
  setprogname (argv[0]);
  app_t *app = app_t::init (argc, argv);
  if (!app) { return -1; }
  else { app->run (); }
  amain ();
  return 0;
}

//=======================================================================

