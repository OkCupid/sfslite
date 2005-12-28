
// -*-c++-*-
/* $Id$ */

#include "tame.h"

#define   TAME_ERROR_SILENT      (1 << 0)
#define   TAME_ERROR_FATAL       (1 << 1)

int tame_options;
int tame_global_int;

int tame_init::count;

void
tame_init::start ()
{
  static bool initialized;
  if (initialized)
    panic ("tame_init called twice\n");
  initialized = true;

  tame_options = 0;
  char *e = safegetenv (TAME_OPTIONS);
  for (char *cp = e; cp && *cp; cp++) {
    switch (*cp) {
    case 'Q':
      tame_options |= TAME_ERROR_SILENT;
      break;
    case 'A':
      tame_options |= TAME_ERROR_FATAL;
      break;
    }
  }
}

void
tame_init::stop ()
{
}

void 
tame_error (const str &loc, const str &msg)
{
  if (!(tame_options & TAME_ERROR_SILENT)) {
    if (loc) {
      warn << loc << ": " << msg << "\n";
    } else 
      warn << msg << "\n";
  }
  if (tame_options & TAME_ERROR_FATAL)
    panic ("abort on TAME failure");
}

void
closure_t::enforce_cceoc (const str &l)
{
  if (_has_cceoc && _cceoc_count != 1) {
    strbuf e ("CEOCC called %d times; expected exactly 1 call!", _cceoc_count);
    tame_error (l, e);
  }
}

void
check_closure_destroyed (str loc, ptr<bool> flag)
{
  if (!*flag) 
    tame_error (loc, "reference to closure leaked");
}


void
closure_t::end_of_scope_checks (str loc)
{
  if (_has_cceoc)
    enforce_cceoc (loc);

  set_weak_finalize_cb (wrap (check_closure_destroyed, loc, 
			      destroyed_flag ()));
  weak_decref ();
}
