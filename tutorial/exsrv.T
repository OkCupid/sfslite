// -*-c++-*-
/* $Id$ */

#include "ex_prot.h"
#include "async.h"
#include "arpc.h"
#include "parseopt.h"
#include "tame.h"
#include "tame_rpcserver.h"

class exsrv_t : public tame::server_t {
public:
  exsrv_t (int fd, int v) : tame::server_t (fd, v) {}
  const rpc_program &get_prog () const { return ex_prog_1; }
  void dispatch (svccb *sbp);
};

class exsrv_factory_t : public tame::server_factory_t {
public:
  exsrv_factory_t () : tame::server_factory_t () {}
  tame::server_t *alloc_server (int fd, int v) { return New exsrv_t (fd, v); }
};

void
reply_rand (svccb *sbp)
{
  u_int i = rand () % 99999;
  sbp->replyref (i);
}

void
reply_rand2 (svccb *sbp)
{
  sbp->replyref (*(sbp->Xtmpl getarg<unsigned> ()) * 3);
}

void
exsrv_t::dispatch (svccb *sbp)
{
  if (!sbp) {
  }

  u_int p = sbp->proc ();
  switch (p) {
  case EX_NULL:
    sbp->reply (NULL);
    break;
  case EX_RANDOM:
    {
      delaycb (rand () % 5, 0, wrap (reply_rand, sbp));
      break;
    }
  case EX_RANDOM2:
    {
      if (*(sbp->Xtmpl getarg<unsigned>()) % 11 == 0) {
	sbp->reject (PROC_UNAVAIL);
      } else {
	delaycb (rand () % 5, 0, wrap (reply_rand2, sbp));
      }
      break;
    }
  case EX_REVERSE:
    {
      ex_str_t *arg = sbp->Xtmpl getarg<ex_str_t> ();
      str s = *arg;
      ex_str_t ret;
      mstr m (s.len ());
      const char *cp = s.cstr ();
      char *mp = m.cstr () + s.len () - 1;
      for ( ; *cp; cp++) {
	*mp = *cp;
	mp--;
      }
      ret = m;
      sbp->replyref (ret);
      break;
    }

  case EX_STRUCT:
    {
      ex_struct_t s;
      s.s = "hello, world!";
      s.u = 34444;
      sbp->replyref (s);
      break;
    }
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool ret;
    exsrv_factory_t fact;
  }
  if (argc != 2)
    fatal << "usage: exsrv <port>\n";

  twait { fact.run (argv[1], mkevent (ret)); }
  exit (ret ? 0 : -1);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
