
// -*-c++-*-
/* $Id$ */

#include "tame_core.h"
#include "tame_thread.h"

int tame_options;

int tame_global_int;
u_int64_t closure_serial_number;
bool tame_collect_jg_flag;
ptr<closure_t> __cls_g;
ptr<closure_t> null_closure;

int tame_init::count;

void
tame_init::start ()
{
  static bool initialized;
  if (initialized)
    panic ("tame_init called twice\n");
  initialized = true;

#ifdef HAVE_PTH
  pth_init ();
#endif

  tame_options = 0;
  closure_serial_number = 0;
  tame_collect_jg_flag = false;
  __cls_g = NULL;
  null_closure = NULL;

  tame_thread_init ();

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

//-----------------------------------------------------------------------
// Methods relating to closures

str
closure_t::loc (int l) const
{
  strbuf b;
  b << _filename << ":" << l << " in function " << _funcname;
  return b;
}

closure_t::closure_t (const char *file, const char *fun)
  : weak_refcounted_t<closure_t> (this),
    _jumpto (0), 
    _id (++closure_serial_number),
    _filename (file),
    _funcname (fun),
    _must_deallocate (New refcounted<must_deallocate_t> ())
  {}

void
closure_t::end_of_scope_checks (int line)
{
  // If we're really concerned about performance, we'll want to disable
  // leak-checking, since checking for leaks does demand registering
  // an extra callback.
  if (tame_check_leaks ()) {

    // Mark all coordination groups automatically allocated in
    // VARS {..} as semi-deallocated.
    semi_deallocate_coordgroups ();

    // After everything unwinds, we can check that everything has
    // been deallocated.
    delaycb (0, 0, wrap (_must_deallocate, &must_deallocate_t::check));
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
closure_t::semi_deallocate_coordgroups ()
{
  for (size_t i = 0; i < _join_groups.size (); i++) {
    _join_groups[i].mark_dead ();
  }
}

void
closure_t::error (int lineno, const char *msg)
{
  str s = loc (lineno);
  tame_error (s.cstr(), msg);
}

void
closure_t::init_block (int blockid, int lineno)
{
  _block._id = blockid;
  _block._count = 1;
  _block._lineno = lineno;
}

ptr<reenterer_t>
closure_t::make_reenter (const char *loc)
{
  ptr<closure_reenter_t> ret = 
    New refcounted<closure_reenter_t> (mkref (this), loc);
  _block._count ++;
  return ret;
}

//
//-----------------------------------------------------------------------

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
// Closure reenterers are interfaces between closures and CVs that the CVs
// carry around.  We're waiting for all of these interfaces to disappear,
// which will prove that the closure reference was flushed, too.
//

void
closure_t::maybe_reenter (const char *loc)
{
  if (block_dec_count (loc))
    reenter ();
}

bool
closure_t::block_dec_count (const char *loc)
{
  bool ret = false;
  if (_block._count <= 0) {
    tame_error (loc, "too many signals for BLOCK environment.");
  } else if (--_block._count == 0) {
    ret = true;
  }
  return ret;
}

void
must_deallocate_t::check ()
{
  qhash<str, int> tab;
  vec<str> lst;
  must_deallocate_obj_t *p;
  for (p = _objs.first ; p ; p = _objs.next (p)) {
    strbuf b;
    str t = p->loc ();
    b << t << ": Object of type '" << p->must_dealloc_typ () << "' leaked";
    str s = b;
    int *n = tab[s];
    if (n) { (*n)++; }
    else { 
      tab.insert (s, 1); 
      lst.push_back (s);
    }
  }

  for (size_t i = 0; i < lst.size (); i++) {
    if (!(tame_options & TAME_ERROR_SILENT)) {
      str s = lst[i];
      warn << s;
      if (*tab[s] > 1) 
	warnx << " (" << *tab[s] << " times)";
      warnx << "\n";
    }
  }

  if (lst.size () > 0 && (tame_options & TAME_ERROR_FATAL))
    panic ("abort on TAME failure\n");
}

closure_reenter_t::closure_reenter_t (ptr<closure_t> c, const char *l)
  : reenterer_t (c->must_deallocate (), l), _cls (c) {}

closure_reenter_t::~closure_reenter_t () {}

reenterer_t::reenterer_t (ptr<must_deallocate_t> m, const char *l)
  : _md (m), _loc (l)
{
  if (_md) _md->add (this);
}

reenterer_t::~reenterer_t ()
{
  if (_md) _md->rem (this);
}

stack_reenter_t::stack_reenter_t (ptr<must_deallocate_t> snt,
				  const char *l, coordgroup_t<> jg)
  : reenterer_t (snt, l), _joiner (jg.make_joiner (l, NULL)) {}

stack_reenter_t::~stack_reenter_t () {}
