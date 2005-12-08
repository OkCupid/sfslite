

#include "ex_prot.h"
#include "async.h"
#include "arpc.h"
#include "parseopt.h"

class exsrv_t {
public:
  void dispatch (svccb *cb);
  exsrv_t (int fd) ;
  ptr<axprt_stream> x;
  ptr<asrv> s;
};

exsrv_t::exsrv_t (int fd)
{
  tcp_nodelay (fd);
  x = axprt_stream::alloc (fd);
  s = asrv::alloc (x, ex_prog_1, wrap (this, &exsrv_t::dispatch));
}

void
reply_rand (svccb *sbp)
{
  u_int i = rand () % 99999;
  sbp->replyref (i);
}

void
exsrv_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    warn << "EOF on socket recevied; shutting down\n";
    delete this;
    return;
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

static bool
init_server (u_int port)
{
  int fd = inetsocket (SOCK_STREAM, port);
  if (!fd) {
    warn << "cannot allocate TCP port: " << port << "\n";
    return false;
  }
  close_on_exec (fd);
  listen (fd, 200);
  fdcb (fd, selread, wrap (new_connection, fd));
  return true;
}

int
main (int argc, char *argv[])
{
  int port;
  if (argc != 2 || !convertint (argv[1], &port))
    fatal << "usage: exsrv <port>\n";

  init_server (port);
  amain ();
}
