// -*-c++-*-
/* $Id$ */

#include "tame_event.h"
#include "tame_closure.h"
#include "tame_event_ag.h"
#include "async.h"
#include "sfs_bundle.h"

void 
_event_cancel_base::cancel ()
{
  _cancelled = true;
  clear ();
  if (_cancel_notifier) {
    ptr<_event_cancel_base> hold (mkref (this));
    if (!_cancel_notifier->cancelled ()) {
      _cancel_notifier->trigger ();
    }
    _cancel_notifier = NULL;
  }
}

void
_event_cancel_base::clear ()
{
  if (!_cleared) {
    clear_action ();
    _cleared = true;
  }
}
