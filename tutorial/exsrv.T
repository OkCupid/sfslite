// -*-c++-*-
/* $Id$ */

#include "ex_prot.h"
#include "async.h"
#include "arpc.h"
#include "parseopt.h"
#include "tame.h"

class exsrv_t {
public:
  void dispatch (svccb *cb);
  void runloop (CLOSURE);
  exsrv_t (int fd) ;
  ptr<axprt_stream> x;
  ptr<asrv> s;
};

exsrv_t::exsrv_t (int fd)
{
  tcp_nodelay (fd);
  x = axprt_stream::alloc (fd);
  runloop ();
}

tamed void
exsrv_t::runloop ()
{
  tvars {
    rendezvous_t<> rv (__FILE__, __LINE__);
    event_t<svccb *>::ptr ev;
    svccb *sbp;
  }

  ev = mkevent (rv, sbp);
  ev->set_reuse (true);
  
  s = asrv_delayed_eof::alloc (x, ex_prog_1, ev);

  do {
    twait (rv);
    if (sbp) {
      dispatch (sbp);
    }
  } while (sbp);

  warn << "EOF on socket recevied; deleting server...\n";
  ev->finish ();

  delete this;
  return;
}

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

static void
new_connection (int lfd)
{
  sockaddr_in sin;
  socklen_t sinlen = sizeof (sin);
  bzero (&sin, sinlen);
  int newfd = accept (lfd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
  if (newfd >= 0) {
    warn ("accepting connection from %s\n", inet_ntoa (sin.sin_addr));
    vNew exsrv_t (newfd);
  } else if (errno != EAGAIN) {
    warn ("accept failure: %m\n");
  }
}

tamed static 
init_server (u_int port, evb_t ev)
{
  tvars {
    int fd;
    event<>::ptr ev;
    bool ret (false);
    bool go (true);
    rendezvous_t<bool> rv (__FILE__, __LINE__);
  }

  fd = inetsocket (SOCK_STREAM, port);
  if (!fd) {
    warn << "cannot allocate TCP port: " << port << "\n";
  } else {
    ret = true;

    sigcb (SIGINT, mkevent (rv, false));
    sigcb (SIGTERM, mkevent (rv, false));

    close_on_exec (fd);
    listen (fd, 200);

    ev = mkevent (rv, true);
    ev->set_reuse (true);
    fdcb (fd, selread, ev);

    while (go) {
      twait (rv, go);
      if (go) 
	new_connection (fd);
    }

    ev->finish ();
  }
  ev->trigger (ret);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool ret;
    int port;
  }
  if (argc != 2 || !convertint (argv[1], &port))
    fatal << "usage: exsrv <port>\n";

  twait { run_server (port, mkevent (ret)); }
  exit (ret ? 0 : -1);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
