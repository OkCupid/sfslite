
// -*-c++-*-
/* $Id$ */

#include "tame.h"

int tame_options;

int tame_global_int;
u_int64_t closure_serial_number;
bool tame_collect_jg_flag;
static recycle_bin_t<ref_flag_t> *rfrb;

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
  tame_collect_jg_flag = false;

  rfrb = New recycle_bin_t<ref_flag_t> ();

  char *e = safegetenv (TAME_OPTIONS);
  for (char *cp = e; cp && *cp; cp++) {
    switch (*cp) {
    case 'Q':
      tame_options |= TAME_ERROR_SILENT;
      break;
    case 'A':
      tame_options |= TAME_ERROR_FATAL;
      break;
    case 'L':
      tame_options |= TAME_CHECK_LEAKS;
      break;
    }
  }
}

void
tame_init::stop ()
{
}

void 
tame_error (const char *loc, const char *msg)
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
closure_t::enforce_cceoc (const char *l)
{
  if (_has_cceoc && _cceoc_count != 1) {
    strbuf e ("CEOCC called %d times; expected exactly 1 call!", _cceoc_count);
    str m (e);
    tame_error (l, m.cstr ());
  }
}

static void
check_closure_destroyed (const char *loc, ptr<ref_flag_t> flag)
{
  if (!*flag) 
    tame_error (loc, "reference to closure leaked");
}


void
closure_t::end_of_scope_checks (const char *loc)
{
  if (_has_cceoc)
    enforce_cceoc (loc);

  // If we're really concerned about performance, we'll want to disable
  // leak-checking, since checking for leaks does demand registering
  // an extra callback.
  if (tame_check_leaks ()) {
    set_weak_finalize_cb (wrap (check_closure_destroyed, loc, 
				destroyed_flag ()));
  
    // potentially weak-decref us if we have join groups that should
    // be going out of scope now.
    kill_join_groups ();

    // decref us for ourselves, since WE should be going out of scope
    // Still need to run this check even for a function that only
    // uses BLOCK { .. }, since it may leak a reference by assigning one
    // of our CVs to a global variable
    weak_decref ();
  }
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

//-----------------------------------------------------------------------
// Mechanism for collecting all allocated join groups, and associating
// them with a closure that's just being allocated

struct collected_join_group_t {
  collected_join_group_t (mortal_ref_t m, const void *p) 
    : _mref (m), _void_p (p) {}
  mortal_ref_t _mref;
  const void  *_void_p;
};

vec<collected_join_group_t> tame_collect_jg_vec;

void
start_join_group_collection ()
{
  tame_collect_jg_flag = true;
  tame_collect_jg_vec.clear ();
}

void
collect_join_group (mortal_ref_t r, void *p)
{
  if (tame_collect_jg_flag) 
    tame_collect_jg_vec.push_back (collected_join_group_t (r, p)); 
}

void
closure_t::collect_join_groups ()
{
  for (u_int i = 0; i < tame_collect_jg_vec.size (); i++) {
    const collected_join_group_t &jg = tame_collect_jg_vec[i];
    associate_join_group (jg._mref, jg._void_p);
  }
  tame_collect_jg_flag = false;
  tame_collect_jg_vec.clear ();
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// recycle ref flags

recycle_bin_t<ref_flag_t> * ref_flag_t::get_recycle_bin () { return rfrb; }

void
ref_flag_t::recycle (ref_flag_t *p)
{
  if (get_recycle_bin ()->add (p)) {
    p->set_can_recycle (false);
  } else {
    delete p;
  }
}

ptr<ref_flag_t>
ref_flag_t::alloc (const bool &b)
{
  ptr<ref_flag_t> ret = get_recycle_bin ()->get ();
  if (ret) {
    ret->set_can_recycle (true);
    ret->set (b);
  } else {
    ret = New refcounted<ref_flag_t> (b);
  }
  return ret;
}


//
//-----------------------------------------------------------------------
