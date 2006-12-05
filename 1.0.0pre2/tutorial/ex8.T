// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "arpc.h"
#include "parseopt.h"
#include "ex_prot.h"

/**
 * Get fastest Web page, where 'fast' is defined by how fast the DNS
 * lookup and TCP session establishment are.  Once connected, request
 * for "/" and dump the response to standard output.
 *
 * @param hosts the hosts to try
 * @param port the port to try on
 * @param done the callback to call when done
 */
tamed static 
void get_web_page (str host, int port, strbuf *resp, cbb done)
{
  tvars {
    int fd, rc;
    strbuf req;
    rendezvous_t<> read_rv, write_rv;
  }

  //
  // get the fastest connection, and dump the result into 'fd'
  //
  twait { tcpconnect (host, port, mkevent (fd)); }
  if (fd < 0) {
    warn << host << ":" << port << ": connection failed\n";
    (*done) (false);
    return;
  }


  //
  // A dirt simple HTTP 1.0 request
  //
  req << "GET / HTTP/1.0\n\n";
  
  //
  // Call 'fdcb' to select on a file descriptor.  In this case we're
  // selecting on the TCP connection to the 'fastest' Web server, and
  // selecting for 'write'.  Once the socket is writeable, we'll be
  // callback.  Note that '@()' generates a callback of type 
  // callback<void> -- i.e., a callback that doesn't 'return' any
  // parameters.
  //
  // Also note that 'fdcb' is a bit different from the asychronous
  // functions we've seen so far, in that it can call us back multiple
  // times.  That is, fdcb will call us back every time the socket
  // 'fd' is available for writing.
  //
  fdcb (fd, selwrite, mkevent (write_rv));
  
  while (true) {
    twait (write_rv);

    //
    // Use this syntax to output the results of the string buffer
    // 'req' to the socket 'fd'. At this point, fdcb has returned,
    // signalling that the socket 'fd' is writable.  If for some
    // reason we were lied to, write() will return <0 inside of
    // suio::output() below, but with errno set to EAGAIN; then 
    // suio::output() will return 0, and we'll try the write again the 
    // next time through the loop.  A return from suio::output() that
    // is negative signals a non-retryable error, and we'll bail out.
    //
    if (req.tosuio ()->output (fd) < 0) {
      warn << "write failed...\n";
      fdcb (fd, selwrite, NULL);
      close (fd);
      (*done) (false);
      return;
    }
    
    //
    // suio::resid() returns the # of bytes left to write.  If there
    // are any bytes left to write, we'll need to be called back from
    // fdcb again, and we'll therefore need to wait again.  Use the
    // coordgroup_t::add_var call to schedule an additional coordination
    // variable for this coordination group.
    //
    if (req.tosuio ()->resid ()) {
      write_rv.add_join ();
    }
    
    // 
    // otherwise, no more writing left to do, and we are no longer
    // interested in writing to 'fd'
    //
    else {
      fdcb (fd, selwrite, NULL);
      break;
    }
  }

  // 
  // As before, but now we need to schedule a callback for reading
  //
  fdcb (fd, selread, mkevent (read_rv));
  
  //
  // The details of the reading loop are almost identical to the
  // writing loop above.
  //
  while (true) {
    twait (read_rv);
    if ((rc = resp->tosuio ()->input (fd)) < 0 && errno != EAGAIN) {
      warn << "read failed...\n";
      fdcb (fd, selread, NULL);
      close (fd);
      TRIGGER (done, false);
      return;
    }
    if (rc != 0) {
      read_rv.add_join ();
    } else {
      fdcb (fd, selread, NULL);
      break;
    }
  }

  close (fd);
  
  // 
  // success!
  //
  TRIGGER (done, true);
}

/**
 * Given a vector of N hosts, connect to all of them on the given port.
 * When the first connection is established, return controle via 'done',
 * and close the remaining stragglers.
 *
 * @param hosts the hosts to try
 * @param port the port to try on
 * @param done the callback to call when the first has returned.
 */
tamed static 
void get_fastest_web_page (vec<str> hosts, int port, cbb done)
{
  tvars {
    u_int i;
    rendezvous_t<u_int, ptr<bool> > rv;
    bool got_one (false);
    vec<strbuf> responses;
    ptr<bool> res;
  }
  
  responses.setsize (hosts.size ());
  
  for (i = 0; i < hosts.size (); i++) {
    res = New refcounted<bool> ();
    get_web_page (hosts[i], port, &responses[i], mkevent (rv,i,res, *res));
  }
  
  while (rv.need_wait ()) {
    twait (rv, i, res);
    warn << hosts[i]  << ":" << port << ": ";
    if (*res) {
      warnx << "connection succeeded";
      if (!got_one) {
	(*done) (true);
	responses[i].tosuio ()->output (1);
	got_one = true;
      } else {
	warnx << "... but too late!";
      }
      warnx << "\n";
    } else {
      warnx << "connection failed\n";
    }
  }
  if (!got_one)
    (*done) (false);
}

static void finish (bool rc)
{
  delaycb (10, 0, wrap (exit, rc ? 0 : -1));
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
