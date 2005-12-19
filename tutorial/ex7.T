// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

TAME(static void connect (vec<str> hosts, int port, cbi done))
{
  // declare all of your "stack" variables here
  VARS {
    int fd (-1);
    u_int i;
    join_group_t<u_int> JG;
    bool got_one (false);
  }

  for (i = 0; i < hosts.size (); i++) {
    NONBLOCK (JG, i) { tcpconnect (hosts[i], port, @(fd)); }
  }
  
  while (JG.need_join ()) {
    JOIN (JG, i) {
      warn << hosts[i]  << ":" << port << ": ";
      if (fd > 0) {
	warnx << "connection succeeded";
	if (!got_one) {
	  (*done) (fd);
	  got_one = true;
	} else {
	  warnx << "... but too late!";
	  close (fd);
	}
	warnx << "\n";
      } else {
	warnx << "connection failed\n";
      }
    }
  }
}

TAME(static void get_fastest_web_page (vec<str> hosts, int port, cbb done))
{
  VARS {
    int fd, rc;
    strbuf req, resp;
    join_group_t<> read_jg, write_jg;
  }

  // get the fastest connection
  BLOCK { connect (hosts, port, @(fd)); }

  req << "GET / HTTP/1.0\n\n";
  
  NONBLOCK (write_jg) { fdcb (fd, selwrite, @()); }
  
  while (true) {
    JOIN (write_jg) {
      if (req.tosuio ()->output (fd) < 0) {
	warn << "write failed...\n";
	fdcb (fd, selwrite, NULL);
	(*done) (false);
	return;
      }
      if (req.tosuio ()->resid ()) 
	write_jg.rejoin ();
      else {
	fdcb (fd, selwrite, NULL);
	break;
      }
    }
  }

  NONBLOCK (read_jg) { fdcb (fd, selread, @()); }

  while (true) {
    JOIN (read_jg) {
      if ((rc = resp.tosuio ()->input (fd)) < 0 && errno != EAGAIN) {
	warn << "read failed...\n";
	fdcb (fd, selread, NULL);
	(*done) (false);
	return;
      }
      if (rc != 0) {
	read_jg.rejoin ();
      } else {
	fdcb (fd, selread, NULL);
	break;
      }
    }
  }

  resp.tosuio ()->output (1);
  (*done) (true);
}

static void finish (bool rc)
{
  delaycb (0, 0, wrap (exit, rc ? 0 : -1));
}

int
main (int argc, char *argv[])
{
  vec<str> hosts;
  int port;
  if (argc < 3 || !convertint (argv[1], &port))
    fatal << "usage: ex2 <port> <host1> <host2> ...\n";

  for (int i = 2; i < argc; i++) 
    hosts.push_back (argv[i]);

  get_fastest_web_page (hosts, port, wrap (finish));

  amain ();
}
