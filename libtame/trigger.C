
#include "tame_trigger.h"

void
dtrigger (callback<void>::ref cb)
{
  delaycb (0, 0, wrap (cb, &callback<void>::trigger));
}
