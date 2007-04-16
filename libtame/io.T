
// -*-c++-*-
#include "tame.h"
#include "tame_core.h"
#include "tame_io.h"
#include "tame_connectors.h"


void
clearread (int fd)
{
  fdcb (fd, selread, NULL);
}

void
clearwrite (int fd)
{
  fdcb (fd, selwrite, NULL);
}

tamed void
fdcb1(int fd, selop which, evv_t cb)
{
  twait { fdcb (fd, which, mkevent ()); }
  fdcb (fd, which, NULL);
  cb->trigger ();
}

tamed void
sigcb1 (int sig, evv_t cb)
{
  twait { sigcb (sig, connector::cnc (mkevent (), cb)); }
  sigcb (sig, NULL);
  cb->trigger ();
}


void
waitwrite (int fd, evv_t cb)
{
  fdcb1(fd, selwrite, cb);
}

void
waitread (int fd, evv_t cb)
{
  fdcb1(fd, selread, cb);
}

tamed void
iofd_t::on (evv_t cb)
{
  _on = true;
  twait { fdcb (_fd, _op, mkevent ()); }
  _on = false;
  cb->trigger ();
  if (!_on) {
    off (false);
  }
}

void
iofd_t::off (bool check)
{
  if (_on || !check) {
    fdcb (_fd, _op, NULL);
    _on = false;
  }
}

typedef enum { READ, WRITE, ENDPROG } ev_t;

void
iofd_sticky_t::on ()
{
  if (!_on && _ev)
    fdcb (_fd, _op, _ev);

  if (_ev) 
    _on = true;
}

void
iofd_sticky_t::off ()
{
  if (_on)
    fdcb (_fd, _op, NULL);

  _on = false;
}

void
iofd_sticky_t::finish ()
{
  off ();
  _ev = NULL;
}



tamed void
proxy (int infd, int outfd, evv_t cb)
{
  tvars {
    rendezvous_t<ev_t> G (__FILE__, __LINE__);
    bool eof (false);
    suio buf;
    size_t sz (0x4000);
    ev_t which;
    int rc;
    bool err (false);
    iofd_sticky_t read (infd, selread), write (outfd, selwrite);
    bool cancelled (false);
  }

  // If the parent process gives up on us, then we'll get a callback here
  cb->set_notify_on_cancel (mkevent (G, ENDPROG));

  read.setev (mkevent (G, READ));
  write.setev (mkevent (G, WRITE));

  while (!err && (!(eof || cancelled) || buf.resid () )) {
    
    if (buf.resid () < sz && !eof) read.on ();
    else read.off ();
    
    if (buf.resid () > 0) write.on ();
    else write.off ();

    twait (G, which);

    switch (which) {
    case READ: 
      if ((rc = buf.input (read.fd (), sz - buf.resid ())) == 0) {
	eof = true;
      } else if (rc < 0 && errno != EAGAIN) {
	warn ("read error: %m\n");
	err = true;
      }
      break;
    case WRITE:
      if ((rc = buf.output (write.fd ())) < 0 && errno != EAGAIN) {
	warn ("write error: %m\n");
	err = true;
      }
      break;
    case ENDPROG:
      cancelled = true;
      break;
    default:
      panic ("unexpected case.\n");
    }
  }

  read.finish ();
  write.finish ();


  cb->trigger ();
  G.cancel (); // for the timer (if necessary )
}

namespace tame {

  tamed void
  read (int fd, char *buf, size_t sz, evi_t ev)
  {
    tvars {
      int rc;
    }
    twait { fdcb (fd, selread, mkevent ()); }
    fdcb (fd, selread, NULL);
    rc = ::read (fd, buf, sz);
    ev->trigger (rc);
  }

  tamed void
  write (int fd, const char *buf, size_t sz, evi_t ev)
  {
    tvars {
      int rc;
      evv_t::ptr ev;
    }
    twait { 
      ev = mkevent ();
      warn ("enable: %p\n", static_cast<_event<> *> (ev));
      fdcb (fd, selwrite, ev);
    }
    warn ("disable: %p\n", static_cast<_event<> *> (ev));
    fdcb (fd, selwrite, NULL);
    rc = ::write (fd, buf, sz);
    ev->trigger (rc);
  }

  tamed void 
  accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen, evi_t ev)
  {
    tvars {
      int rc;
    }
    twait { fdcb (sockfd, selread, mkevent ()); }
    fdcb (sockfd, selread, NULL);
    rc = ::accept (sockfd, addr, addrlen);
    ev->trigger (rc);
  }




};
