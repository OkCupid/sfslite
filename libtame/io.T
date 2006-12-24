
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

tamed static void
fdcb1(int fd, selop which, cbv cb, ptr<canceller_t> *cnp)
{
  tvars {
    rendezvous_t<bool> G (__FILE__, __LINE__);
    ptr<canceller_t> cn;
    bool ok;
  }
  if (cnp) {
    cn = New refcounted<canceller_t> ();
    *cnp = cn;
    cn->wait (mkevent (G, false));
  }
  fdcb (fd, which, mkevent (G, true));
  twait (G, ok);
  fdcb (fd, which, NULL);
  if (ok) {
    TRIGGER (cb);
  }
  twait { G.gc (mkevent ()); }
}


tamed void
waitwrite (int fd, cbv cb, ptr<canceller_t> *cnp)
{
  fdcb1(fd, selwrite, cb, cnp);
}

tamed void
waitread (int fd, cbv cb, ptr<canceller_t> *cnp)
{
  fdcb1(fd, selread, cb, cnp);
}
