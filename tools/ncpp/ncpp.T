// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "parseopt.h"
#include "tame.h"

//=======================================================================

struct app_t {
  static app_t *init (int argc, char *argv[]);
  app_t (int port) : _port (port) {}
  static void usage ();
  virtual void run (CLOSURE) = 0;
protected:
  int _port;
};

//=======================================================================

class client_t : public app_t {
public:
  client_t (str h, int p) : app_t (p), _host (h) {}
  void run (CLOSURE) {}
protected:
  str _host;
};

//=======================================================================

class server_t : public app_t {
public:
  server_t (int p, str file) : app_t (p), _file (file) {}
  void run (CLOSURE) {}
protected:
  str _file;
};

//=======================================================================

app_t *
app_t::init (int argc, char *argv[])
{
  int ch;
  str file;
  bool err = false;
  app_t *ret = NULL;
  int port = -1;
  str hostname;

  while ((ch = getopt (argc, argv, "p:"))) {
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
    ret = New client_t (hostname, port);
  } else {
    ret = New server_t (port, file);
  }

  return NULL;
}

//-----------------------------------------------------------------------

void
app_t::usage ()
{
  warnx << "usage: " << progname << " [-f <file>] -p <port> [<hostname>]\n"
	<< "  - run as a server without the hostname argument\n"
	<< "  - run as a client with the hostname arugment\n"
	<< "  - specify -p for port in both cases\n"
	<< "  - specify a <file> -f to output to a file\n"
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

