
// -*-c++-*-
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
waitread (int fd, cbv cb)
{
  twait { fdcb (fd, selread, mkevent ()); }
  clearread (fd);
  cb->trigger ();
}

tamed void
waitwrite (int fd, cbv cb)
{
  twait { fdcb (fd, selwrite, mkevent ()); }
  clearwrite (fd);
  cb->trigger ();
}
