
// -*-c++-*-
#include "tame.h"
#include "tame_core.h"
#include "tame_cancel.h"
#include "tame_io.h"

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
  twait { sigcb (sig, mkevent ()); }
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

tamed void
proxy (int infd, int outfd, evv_t cb, ptr<canceller_t> *cncp)
{
  tvars {
    rendezvous_t<ev_t> G (__FILE__, __LINE__);
    bool eof (false);
    suio buf;
    size_t sz (0x4000);
    ev_t which;
    int rc;
    bool err (false);
    iofd_t read (infd, selread), write (outfd, selwrite);
    ptr<canceller_t> cnc;
    bool cancelled (false);
  }

  if (cncp) {
    *cncp = cnc = New refcounted<canceller_t> ();
    cnc->wait (mkevent (G, ENDPROG));
  }

  while (!err && (!(eof || cancelled) || buf.resid () )) {
    
    if (buf.resid () < sz && !eof) read.on (mkevent (G, READ));
    else read.off ();
    
    if (buf.resid () > 0) write.on (mkevent (G, WRITE));
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
      cnc->wait (mkevent (G, ENDPROG));
      cancelled = true;
      break;
    default:
      panic ("unexpected case.\n");
    }
    read.off ();
    write.off ();
  }
  if (!cancelled && cnc)
    cnc->toolate ();

  cb->trigger ();
  G.cancel (); // for the timer (if necessary )
}
