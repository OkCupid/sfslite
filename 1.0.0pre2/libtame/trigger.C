

#include "tame_trigger.h"

#ifdef SFS_HAVE_CALLBACK2

void
dtrigger (callback<void>::ref cb)
{
  delaycb (0, 0, wrap (cb, &callback<void>::trigger));
}

#endif // SFS_HAVE_CALLBACK2
