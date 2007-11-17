
// -*-c++-*-
#include "tame.h"
#include "tame_io.h"
#include "tame_connectors.h"

namespace tame {

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
proxy (int infd, int outfd, evv_t ev)
{
  tvars {
    std_proxy_t px;
  }
  twait { px.go (infd, outfd, mkevent ()); }
  ev->trigger ();
}

bool
std_proxy_t::have_room_to_read () const
{
  return (_sz > _buf.resid ());
}

bool
std_proxy_t::have_data_to_write () const 
{
  return (_buf.resid () > 0);
}

int
std_proxy_t::v_read (int fd)
{
  return _buf.input (fd, _sz - _buf.resid ());
}

int 
std_proxy_t::v_write (int fd)
{
  return _buf.output (fd);
}

std_proxy_t::std_proxy_t (size_t s) : _sz (s) {}
std_proxy_t::~std_proxy_t () {}

tamed void
proxy_t::go (int infd, int outfd, evv_t ev)
{
  tvars {
    rendezvous_t<ev_t> rv (__FILE__, __LINE__);
    bool eof (false);
    ev_t which;
    int rc;
    bool err (false);
    iofd_sticky_t read (infd, selread), write (outfd, selwrite);
    bool cancelled (false);
  }

  // If the parent process gives up on us, then we'll get a callback here
  ev->set_cancel_notifier (mkevent (rv, ENDPROG));

  read.setev (mkevent (rv, READ));
  write.setev (mkevent (rv, WRITE));

  while (!err && (!(eof || cancelled) || have_data_to_write ())) {
    
    if (have_room_to_read () && !eof) read.on ();
    else read.off ();
    
    if (have_data_to_write ()) write.on ();
    else write.off ();

    twait (rv, which);

    switch (which) {
    case READ: 
      if ((rc = v_read (read.fd())) == 0) {
	eof = true;
      } else if (rc < 0 && errno != EAGAIN) {
	warn ("read error: %m\n");
	err = true;
      }
      break;
    case WRITE:
      if ((rc = v_write (write.fd())) < 0 && errno != EAGAIN) {
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

  ev->trigger ();
  rv.cancel (); // for the timer (if necessary )
}

tamed void
read (int fd, char *buf, size_t sz, evi_t ev)
{
  tvars { int rc; }

  twait { fdcb (fd, selread, mkevent ()); }
  fdcb (fd, selread, NULL);
  rc = ::read (fd, buf, sz);
  ev->trigger (rc);
}

tamed void
write (int fd, const char *buf, size_t sz, evi_t ev)
{
  tvars { int rc; }

  twait { fdcb (fd, selwrite, mkevent ()); }
  fdcb (fd, selwrite, NULL);
  rc = ::write (fd, buf, sz);
  ev->trigger (rc);
}

tamed void 
accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen, evi_t ev)
{
  tvars { int rc; }
  
  twait { fdcb (sockfd, selread, mkevent ()); }
  fdcb (sockfd, selread, NULL);
  rc = ::accept (sockfd, addr, addrlen);
  ev->trigger (rc);
}

};
