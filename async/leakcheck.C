
#include "sysconf.h"

#if !defined(DMALLOC) and defined(SIMPLE_LEAK_CHECKER)

#include "async.h"
#include "ihash.h"
#include "sfs_profiler.h"

//-----------------------------------------------------------------------

static bool slc_enabled;
simple_leak_checker_t simple_leak_checker;
#define PRFX "(SLC) "

//-----------------------------------------------------------------------

class malloc_id_t {
public:
  malloc_id_t (const char *f, int l) : _file (f), _line (l) {}
  hash_t to_hash () const;
  bool operator== (const malloc_id_t &m) const { return eq (m); }
  operator hash_t() const { return to_hash (); }
  bool eq (const malloc_id_t &m) const;

  const char *_file;
  int _line;
};

//-----------------------------------------------------------------------

class malloc_site_t {
public:
  malloc_site_t (const malloc_id_t &id)
    : _id (id), _total (0), _n (0) {}

  void allocate (size_t s);
  void deallocate (size_t s);
  void report () const;

  malloc_id_t _id;
  ihash_entry<malloc_site_t> _lnk;

  size_t _total;
  size_t _n;
};

//-----------------------------------------------------------------------

class malloc_atom_t {
public:
  malloc_atom_t (void *p, size_t z, malloc_site_t *s)
    : _ptr (p), _size (z), _site (s) {}
  
  void deallocate () { _site->deallocate (_size); }

  void *_ptr;
  const size_t _size;
  malloc_site_t *_site;

  ihash_entry<malloc_atom_t> _lnk;
};

//-----------------------------------------------------------------------

template<> 
struct equals<void *>
{
  equals () {}
  bool operator() (const void *a, const void *b) const { return a == b; }
};

template<> 
struct hashfn<void *>
{
  hashfn () {}
  hash_t operator() (const void *v)  const 
  { return reinterpret_cast<intptr_t> (v); }
};

//-----------------------------------------------------------------------

class malloc_tracker_t {
public:
  malloc_tracker_t () : _internal (false) {}
  ~malloc_tracker_t () { _internal = true; }

  void allocate (void *p, size_t s, const char *f, int l);
  void deallocate (void *p);
  void report ();
  void clear ();

  typedef ihash<malloc_id_t, malloc_site_t, 
		&malloc_site_t::_id, &malloc_site_t::_lnk> sites_t;
  sites_t _sites;

  ihash<void *, malloc_atom_t,
	&malloc_atom_t::_ptr, &malloc_atom_t::_lnk> _atoms;

private:
  bool _internal;
};

static malloc_tracker_t tracker;

//-----------------------------------------------------------------------

#define MT_ENTER() bool __i = _internal; _internal = true 
#define MT_EXIT() _internal = __i
#define MT_SHORT_CIRCUIT_REENTRY() if (_internal) return; MT_ENTER()

//-----------------------------------------------------------------------

void
malloc_tracker_t::allocate (void *p, size_t s, const char *f, int l)
{
  MT_SHORT_CIRCUIT_REENTRY();

  malloc_id_t id (f, l);
  malloc_atom_t *atom;

  if ((atom = _atoms[p])) {
    panic ("Pointer was allocated twice!! %p\n", p);
  }

  malloc_site_t *site = _sites[id];
  if (!site) {
    site = new malloc_site_t (id);
    _sites.insert (site);
  }

  atom = new malloc_atom_t (p, s, site);
  _atoms.insert (atom);
  site->allocate (s);

  MT_EXIT();
}

//-----------------------------------------------------------------------

void
malloc_site_t::deallocate (size_t s)
{
  if (_n > 0) {  _n--; }

  if (s <= _total) { 
    _total -= s; 
  } else {
    warn (PRFX "Size underrun (total=%zu,size=%zu) for %s:%d\n", 
	  _total, s, _id._file, _id._line);
  }
}

//-----------------------------------------------------------------------

void
malloc_site_t::allocate (size_t s)
{
  _n ++;
  _total += s;
}

//-----------------------------------------------------------------------

void
malloc_tracker_t::deallocate (void *p)
{
  MT_SHORT_CIRCUIT_REENTRY();

  malloc_atom_t *a = _atoms[p];
  if (a) {
    _atoms.remove (a);
    a->deallocate ();
    delete a;
  }

  MT_EXIT();
}

//-----------------------------------------------------------------------


hash_t 
malloc_id_t::to_hash () const
{
  intptr_t i = reinterpret_cast<intptr_t> (_file);
  hash_t h1 = i;
  if (h1 < (1 << 16)) {
    h1 = h1 | (h1 << 16);
  }
  hash_t h2 = _line;
  h2 = h2 | (h2 << 12) | (h2 << 12);
  return h1 ^ h2;
}

//-----------------------------------------------------------------------

bool
malloc_id_t::eq (const malloc_id_t &m) const
{
  return (_file == m._file && _line == m._line);
}

//-----------------------------------------------------------------------

static int
scmp (const void *va, const void *vb)
{
  const malloc_site_t *const *a 
    = reinterpret_cast<const malloc_site_t *const *> (va);
  const malloc_site_t *const *b 
    = reinterpret_cast<const malloc_site_t *const *> (vb);
  return ((*b)->_total - (*a)->_total);
}

//-----------------------------------------------------------------------

void
malloc_tracker_t::report () 
{
  MT_ENTER();
  
  typedef const malloc_site_t *p_t;
  size_t l = _sites.size ();
  const malloc_site_t **v = new p_t[l];
  ihash_iterator_t<malloc_site_t, sites_t> it (_sites);
  size_t i = 0;

  const malloc_site_t *p;
  while ((p = it.next ())) { v[i++] = p; }
  assert (i == l);

  qsort (v, l, sizeof (malloc_site_t *), scmp);
  for (const malloc_site_t **p = v; p < v + l; p++) {
    (*p)->report ();
  }
  delete [] v;

  MT_EXIT();
}

//-----------------------------------------------------------------------

void
malloc_tracker_t::clear ()
{
  MT_ENTER ();
  _atoms.deleteall ();
  _sites.deleteall ();
  MT_EXIT ();
}

//-----------------------------------------------------------------------

void
malloc_site_t::report () const
{
  if (_total > 0 || _n > 0) {
    warn << PRFX << _total << " " << _n << " " 
	 << _id._file << ":" << _id._line << "\n";
  }
}

//-----------------------------------------------------------------------

void
simple_leak_checker_enable ()
{
  slc_enabled = true;
}

//-----------------------------------------------------------------------

void
simple_leak_checker_disable ()
{
  simple_leak_checker_reset ();
  slc_enabled = false;
}

//-----------------------------------------------------------------------

void 
simple_leak_checker_reset ()
{
  tracker.clear ();
}

//-----------------------------------------------------------------------

void
simple_leak_checker_report ()
{
  tracker.report ();
}

//-----------------------------------------------------------------------

//=======================================================================
//=======================================================================
//=======================================================================
//=======================================================================

static void
default_xmalloc_handler (int size)
{
  const char msg[] = "malloc failed\n";
  v_write (errfd, msg, sizeof (msg) - 1);
  myabort ();
}

//-----------------------------------------------------------------------

void *
_internal_new (size_t s, const char *f, int l)
{
  if (!s) s = 1;
  sfs_profiler::enter_vomit_lib ();
  void *p = malloc (s);
  sfs_profiler::exit_vomit_lib ();
  if (slc_enabled) {
    tracker.allocate (p, s, f, l);
  }
  if (!p) default_xmalloc_handler (s);
  return p;
}

//-----------------------------------------------------------------------

void
_internal_free (void *p)
{
  if (slc_enabled) {
    tracker.deallocate (p);
  }
  sfs_profiler::enter_vomit_lib ();
  free (p);
  sfs_profiler::exit_vomit_lib ();
}

//-----------------------------------------------------------------------

void *
operator new (size_t s, simple_leak_checker_t, const char *f, int l)
{
  return _internal_new (s, f, l);
}

//-----------------------------------------------------------------------

void *operator 
new[] (size_t s, simple_leak_checker_t, const char *f, int l)
{
  return _internal_new (s, f, l);
}

//-----------------------------------------------------------------------

void *operator 
new (size_t s, nothrow_t, const char *f, int l) throw ()
{
  return _internal_new (s, f, l);
}

//-----------------------------------------------------------------------

void *operator 
new[] (size_t s, nothrow_t, const char *f, int l) throw ()
{
  return _internal_new (s, f, l);
}

//-----------------------------------------------------------------------

void 
simple_leak_checker_free (void *p)
{
  _internal_free (p);
}

//-----------------------------------------------------------------------

void *
simple_leak_checker_malloc (const char *fl, int line, size_t sz)
{
  return _internal_new (sz, fl, line);
}

//-----------------------------------------------------------------------

#ifndef delete_throw
# define delete_throw throw()
#endif /* !delete_throw */

//-----------------------------------------------------------------------

void
operator delete (void *ptr) delete_throw
{
  _internal_free (ptr);
}

//-----------------------------------------------------------------------

void
operator delete[] (void *ptr) delete_throw
{
  _internal_free (ptr);
}

//-----------------------------------------------------------------------

#endif /* !DMALLOC && SIMPLE_LEAK_CHECKER */
