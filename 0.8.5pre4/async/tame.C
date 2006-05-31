
// -*-c++-*-
/* $Id$ */

#include "tame.h"

#define   TAME_ERROR_SILENT      (1 << 0)
#define   TAME_ERROR_FATAL       (1 << 1)

int tame_options;
int tame_global_int;
u_int64_t closure_serial_number;

int tame_init::count;

void
tame_init::start ()
{
  static bool initialized;
  if (initialized)
    panic ("tame_init called twice\n");
  initialized = true;

  tame_options = 0;
  closure_serial_number = 0;
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

static void
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

  // potentially decref us if we have join groups that should
  // be going out of scope now.
  kill_join_groups ();

  // decref us for ourselves, since WE should be going out of scope
  weak_decref ();
}

void
closure_t::associate_join_group (mortal_ref_t mr, const void *jgwp)
{
  if (is_onstack (jgwp)) {
    _join_groups.push_back (mr);
  }
}

void
closure_t::kill_join_groups ()
{
  for (size_t i = 0; i < _join_groups.size (); i++) {
    _join_groups[i].mark_dead ();
  }
}

void 
mortal_ref_t::mark_dead ()
{
  if (!*_destroyed_flag)
    _mortal->mark_dead ();
}
