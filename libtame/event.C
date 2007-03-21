// -*-c++-*-
/* $Id$ */

#include "tame_event.h"
#include "tame_core.h"
#include "async.h"

void
callback_second_trigger (const char *loc)
{
  tame_error (loc, "event triggered after deallocation");
}
