// -*-c++-*-

#include "tame_profiler.h"

namespace tame {

#if 0
} // fool emacs
#endif

//-----------------------------------------------------------------------

ustime_t::ustime_t () { set (sfs_get_tsnow ()); }

//-----------------------------------------------------------------------

void
ustime_t::set (const struct timespec &ts)
{
  u_int64_t mil = 1000000;
  u_int64_t thou = 1000;
  _time = u_int64_t (ts.tv_sec) * thou + u_int64_t (ts.tv_nsec) / mil;
}

//-----------------------------------------------------------------------

profiler_t::profiler_t () : _enabled (false) {}

//-----------------------------------------------------------------------

void profiler_t::enable () { _enabled = true; }

//-----------------------------------------------------------------------

void profiler_t::disable () { _enabled = false; clear (); } 

//-----------------------------------------------------------------------

void
profiler_t::clear ()
{
  _tab.deleteall ();
}

//-----------------------------------------------------------------------

void
profiler_t::exit_closure (const closure_t *c)
{
  if (!_enabled) return;
  profile_site_t *s = _tab[c->loc ()];
  if (s) { s->remove_instance (c); }
}

//-----------------------------------------------------------------------

void
profiler_t::enter_closure (const closure_t *c)
{
  if (!_enabled) return;
  str l = c->loc ();
  profile_site_t *s = _tab[l];
  if (!s) {
    s = New profile_site_t (l, c->funcname ());
    _tab.insert (s);
  }
  s->add_instance (c);
}

//-----------------------------------------------------------------------

void
profile_site_t::add_instance (const closure_t *c)
{
  _instances.insert (c->id ());
}

//-----------------------------------------------------------------------

void
profile_site_t::remove_instance (const closure_t *c)
{
  _instances.remove (c->id ());
}

//-----------------------------------------------------------------------

static profiler_t g_profiler;
profiler_t *profiler_t::profiler () { return &g_profiler; }

//-----------------------------------------------------------------------

static int
qcmp (const void *va, const void *vb)
{
  const profile_site_t *const *a = 
    reinterpret_cast<const profile_site_t * const*> (va);
  const profile_site_t *const *b = 
    reinterpret_cast<const profile_site_t * const *> (vb);
  return (*b)->_msec_avg - (*a)->_msec_avg;
}

//-----------------------------------------------------------------------

void
profile_site_t::prep_report ()
{
  ustime_t now;
  _n = _instances.size ();
  _msec_tot = 0;
  
  qhash_const_iterator_t<u_int64_t, ustime_t> it (_instances);
  const u_int64_t *key;
  ustime_t val;
  
  while ((key = it.next (&val))) {
    _msec_tot += (now._time - val._time);
  }
  if (_n) {
    _msec_avg = _msec_tot / _n;
  } else {
    _msec_avg = 0;
  }
}

//-----------------------------------------------------------------------

static const char *prefix = "(STP): ";

//-----------------------------------------------------------------------

void
profile_site_t::report ()
{
  if (_n) {
    warn << prefix << _msec_avg << " " << _n << " " 
	 << _loc << " " << _func << "\n";
  }
}

//-----------------------------------------------------------------------

void
profiler_t::report ()
{
  ihash_iterator_t<profile_site_t, tab_t> it (_tab);
  profile_site_t *site;
  vec<profile_site_t *> sites;
  sites.reserve (_tab.size ());
  while ((site = it.next ())) {
    sites.push_back (site);
    site->prep_report ();
  }
  qsort (sites.base (), sites.size (), sizeof (profile_site_t *), qcmp);
 
  warn << prefix << "Start dump ===================================\n";
  for (size_t i = 0; i < sites.size (); i++) {
    sites[i]->report ();
  }
}

//-----------------------------------------------------------------------

}

