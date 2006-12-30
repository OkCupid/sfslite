
// -*-c++-*-
/* $Id: tcpconnect.T 2272 2006-10-27 02:25:59Z max $ */

#include "async.h"
#include "tame.h"
#include "tame_io.h"
#include "rxx.h"
#include "parseopt.h"

static void
usage ()
{
  warnx << "usage: progname <localport> <hostname>:<port>\n";
  exit (-1);
}

static int
listen_on (int port)
{
  int fd = inetsocket (SOCK_STREAM, port);
  if (fd >= 0) {
    close_on_exec (fd);
    listen (fd, 20);
  } else {
    warn ("listen failed on port %d: %m\n", port);
  }
  return fd;
}

typedef enum { PROXY_DONE, PROXY_END, READ, WRITE } ev_t;

tamed static void
do_proxy (int infd, str hn, int port, cbv cb)
{
  tvars {
    int outfd;
  }
  warn << "TCPconnect to " << hn << ":" << port << "\n";
  twait { tcpconnect (hn, port, mkevent (outfd)); }
  if (outfd >= 0) {
    twait {
      proxy (infd, outfd, mkevent ());
      proxy (outfd, infd, mkevent ());
    }
  } else {
    warn ("tcpconnect failed: %m\n");
  }
  cb->trigger ();
}

tamed static void
listen_loop (int lport, str hn, int port, ptr<canceller_t> *cnp, cbv cb)
{
  tvars {
    int lfd;
    bool go (true);
    rendezvous_t<ev_t> G (__FILE__, __LINE__);
    ptr<canceller_t> cncl, rcnc;
    int nfd;
    ev_t which;
    int proxies_out (0);
    sockaddr_in sin;
    socklen_t sinlen (sizeof (sin));
  }

  if ((lfd = listen_on (lport)) >= 0) {

    cncl = New refcounted<canceller_t> ();
    cncl->wait (mkevent (G, PROXY_END));
    *cnp = cncl;

    while (go || proxies_out > 0 ) {

      if (go && !rcnc)
	waitread (lfd, mkevent (G, READ), &rcnc);

      twait (G, which);

      switch (which) {
      case READ:
	{
	  bzero (&sin, sinlen);
	  if ((nfd = accept (lfd, reinterpret_cast<sockaddr *> (&sin), 
			     &sinlen)) >= 0) {
	    warn ("accepting connection from %s\n", inet_ntoa (sin.sin_addr));
	    proxies_out ++;
	    do_proxy (nfd, hn, port, mkevent (G, PROXY_DONE));
	  } else {
	    warn ("accept failed: %m\n");
	  }
	  rcnc = NULL;
	  break;
	}
      case PROXY_DONE:
	proxies_out --;
	break;
      case PROXY_END:
	warn << "Caught shutdown signal...\n";
	go = false;
	break;
      default:
	panic ("weird\n");
      }
    }

    warn << "Exiting listen loop....\n";
    if (rcnc) rcnc->cancel ();

    twait { G.gc (mkevent ()); }
  }
  cb->trigger ();
}

tamed static void
main2 (int lport, str hn, int port)
{
  tvars {
    ptr<canceller_t> cnc;
  }

  twait {
    listen_loop (lport, hn, port, &cnc, mkevent ());
    if (cnc) {
      sigcb (SIGINT, wrap (cnc, &canceller_t::cancel));
      sigcb (SIGTERM, wrap (cnc, &canceller_t::cancel));
    }
  }
  exit (0);
}


int 
main (int argc, char *argv[])
{
  static rxx x ("([^:]+):(\\d+)");
  int lport;
  int port;
  str hn;
  setprogname (argv[0]);

  if (argc == 3 && convertint (argv[1], &lport) && 
      x.match (argv[2]) && convertint (x[2], &port) &&
      (hn = x[1])) {
    main2 (lport, hn, port);
  } else {
    usage ();
  }
  amain ();
}

