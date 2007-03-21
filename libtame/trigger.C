
#include "tame_trigger.h"

void
dtrigger (event_t<>::ref cb)
{
  delaycb (0, 0, wrap (cb, &event<>::trigger));
}

