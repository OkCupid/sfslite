// -*-c++-*-
/* $Id: ex4.T 2648 2007-03-29 19:33:14Z max $ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

static void
fillarg (vsize_t *arg, size_t outsz)
{
  arg->buf.setsize (outsz);
  char *bp = arg->buf.base ();
  const char *ep = bp + outsz;
  int i = 0;
  while (bp < ep) {
    *bp++ = (i++) & 0xff;
  }
}

tamed static void 
go (const str &h, int port, int ncalls, int outsz, cbb ev)
{
  tvars {
    int fd (99999);
    ptr<axprt_stream> x;
    ptr<aclnt> cli;
    int i;
    vsize_t arg, res;
    clnt_stat err;
    bool ok (true);
  }

  twait { tcpconnect (h, port, mkevent(fd)); }

  if (fd < 0) {
    warn ("%s:%d: connection failed: %m\n", h.cstr(), port);
    ok = false;
  } else {

    x = axprt_stream::alloc (fd);
    cli = aclnt::alloc (x, ex_prog_1);

    for (i = 0; ok && i < ncalls; i++) {
      if (i % 10000 == 0) {
	warn << sfs_get_timenow () << ": " << i << "\n";
      }
      fillarg (&arg, outsz);
      twait { RPC::ex_prog_1::ex_perftest (cli, arg, &res, mkevent (err)); }
      if (err) {
	warn << "Error in client call: " << err << "\n";
	ok = false;
      }
    }
    warn << "All done...\n";
  }
  ev->trigger (ok);
}

static void finish (bool rc)
{
  exit (rc ? 0 : -1);
}

static void usage ()
{
  warnx << "usage: " << progname << " [-h <host>] [-p <port>]"
	<< " [-s <packetsize] [-n <n-packets>]\n";
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    int ch;
    str hostname;
    int port (2000);
    int n_calls (1000000);
    int outsz (0x100);
    bool ok;
  }

  while ((ch = getopt (argc, argv, "h:p:s:n:")) != -1) {
    switch (ch) {
    case 'h':
      hostname = optarg;
      break;
    case 'n':
      if (!convertint (optarg, &n_calls)) {
	fatal << "Bad number of calls: " << optarg << "\n";
      }
      break;
    case 'p':
      if (!convertint (optarg, &port)) {
	fatal << "Bad port: " << optarg << "\n";
      }
      break;
    case 's':
      if (!convertint (optarg, &outsz)) {
	fatal << "Bad output packet size: " << optarg << "\n";
      }
      break;
    default:
      usage ();
    }
  }

  if (!hostname) {
    fatal << "No hostname specified for server\n";
  }
  
  warn ("[%d] Starting up: server=%s:%d; ncalls=%d; packetsz=%d\n",
	getpid (), hostname.cstr (), port, n_calls, outsz);
  
  twait { go (hostname, port, n_calls, outsz, mkevent (ok)); }
  finish (ok);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}

