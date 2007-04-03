// -*-c++-*-
/* $Id$ */

#include "tame_event.h"
#include "tame_closure.h"
#include "tame_event_ag.h"
#include "async.h"

void 
_event_cancel_base::cancel ()
{
  _cancelled = true;
  if (_cancel_notifier) {
    ptr<_event_cancel_base> hold;
    if (!_cancel_notifier->cancelled ()) {
      hold = mkref (this);
      _cancel_notifier->trigger ();
    }
    _cancel_notifier = NULL;
  }
}
