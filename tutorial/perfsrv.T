// -*-c++-*-
/* $Id: perfsrv.T 3495 2008-08-07 00:45:59Z max $ */

#include "ex_prot.h"
#include "async.h"
#include "arpc.h"
#include "parseopt.h"
#include "tame.h"
#include "tame_rpcserver.h"
#include "rpc_stats.h"

class perfsrv_t : public tame::server_t {
public:
  perfsrv_t (int fd, int v) : tame::server_t (fd, v) {}
  const rpc_program &get_prog () const { return ex_prog_1; }
  void dispatch (svccb *sbp);
};

class perfsrv_factory_t : public tame::server_factory_t {
public:
  perfsrv_factory_t () : tame::server_factory_t () {}
  tame::server_t *alloc_server (int fd, int v) { return New perfsrv_t (fd, v); }
};

size_t g_out_size;
u_int g_port;
size_t n_calls;

tamed static void report_loop  ()
{
  while (true) {
    twait { delaycb (5, 0, mkevent ()); }
    warn << sfs_get_timenow () << " " << n_calls << "\n";
  }
}

void
perfsrv_t::dispatch (svccb *sbp)
{
  switch (sbp->proc ()) {
  case EX_PERFTEST:
    {
      vsize_t resp;
      const vsize_t *arg;
      RPC::ex_prog_1::ex_perftest_srv_t<svccb> x (sbp);
      arg = x.getarg ();

      resp.buf.setsize (g_out_size);
      char *bp = resp.buf.base ();
      const char *ep = bp + g_out_size;

      while (bp < ep) {
	size_t bytes = min<size_t> (ep - bp, arg->buf.size ());
	memcpy (bp, arg->buf.base (), bytes);
	bp += bytes;
      }
      x.reply (resp);
      n_calls ++;
    }
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

static void
usage ()
{
  warnx << "usage: " << progname << " [-p <port>] [-s<packetsize]\n";
  exit (1);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool ret;
    perfsrv_factory_t fact;
    int ch;
  }
  if (argc != 2)
    fatal << "usage: perfsrv <port>\n";

  g_out_size = 10;
  g_port = 2000;

  while ((ch = getopt (argc, argv, "p:s:")) != -1) {
    switch (ch) {
    case 'p':
      if (!convertint (optarg, &g_port)) {
	fatal << "bad port: " << optarg << "\n";
      }
      break;
    case 's':
      if (!convertint (optarg, &g_out_size)) {
	fatal << "Bad output packet size: " << optarg << "\n";
      }
      break;
    default:
      usage ();
      break;
    }
  }

  warn << "+ Starting up; port=" << g_port 
       << "; output packet size=" << g_out_size << "\n";

  report_loop ();
  twait { fact.run (g_port, mkevent (ret)); }

  exit (ret ? 0 : -1);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
