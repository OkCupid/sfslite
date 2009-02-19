
#include "sysconf.h"

#ifdef SIMPLE_PROFILER

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
#include <ucontext.h>

#include "sfs_profiler.h"
#include <dlfcn.h>
#include "ihash.h"

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

//-----------------------------------------------------------------------

typedef u_long my_intptr_t;

//-----------------------------------------------------------------------

class obj_file_t {
public:
  obj_file_t (const char *c, const void *b) 
    : _name (c), _id (global_id++), _base(b), _main (false) {}
  static u_int32_t global_id;
  u_int32_t id () const { return _id; }
  void report () const;
  void set_as_main () { _main = true; }

  const char *_name;
  ihash_entry<obj_file_t> _lnk;
private:
  const u_int32_t _id;
  const void *_base;
  volatile bool _main;
};

u_int32_t obj_file_t::global_id;

//-----------------------------------------------------------------------

class call_site_key_t {
public:
  call_site_key_t (obj_file_t *f, my_intptr_t o) 
    : _file (f), _offset (o) {}

  my_intptr_t offset () const { return _offset; }
  const obj_file_t *file () const { return _file; }
  obj_file_t *file () { return _file; }
  bool operator== (const call_site_key_t &b) const
  { return _offset == b.offset () && _file->id () == b.file ()->id (); }

private:
  obj_file_t *const _file;
  const my_intptr_t _offset;
};

//-----------------------------------------------------------------------

class call_site_t {
public:
  call_site_t (const call_site_key_t &k) : _key (k) , _id (global_id++) {}

  static u_int32_t global_id;
  u_int32_t id () const { return _id; }
  void report () const;
  void set_as_main () { _key.file ()->set_as_main (); }

  call_site_key_t _key;
  ihash_entry<call_site_t> _lnk;
private:
  const u_int32_t _id;
};

u_int32_t call_site_t::global_id;

//-----------------------------------------------------------------------

template<>
struct hashfn<call_site_key_t>
{
  hashfn () {}
  hash_t operator() (const call_site_key_t &in)  const
  { return in.offset () * in.file ()->id (); }
};

template<>
struct equals<call_site_key_t>
{
  equals () {}
  bool operator() (const call_site_key_t &a, const call_site_key_t &b) const
  { return a == b; }
};

//-----------------------------------------------------------------------

class call_table_t {
public:
  call_table_t () {}

  call_site_t *lookup (const char *file, my_intptr_t p, const void *b);

  void reset () { _sites.deleteall (); _files.deleteall (); }
  void report ();
  void report_sites ();
  void report_files ();

  typedef ihash<const char *, obj_file_t, 
		&obj_file_t::_name, &obj_file_t::_lnk> file_tab_t;
  typedef ihash<call_site_key_t, call_site_t,
		&call_site_t::_key, &call_site_t::_lnk> site_tab_t;
private:
  file_tab_t _files;
  site_tab_t _sites;
}; 

//-----------------------------------------------------------------------

class edge_key_t {
public:
  edge_key_t (const call_site_t *caller, const call_site_t *callee)
    : _caller (caller->id ()), _callee (callee->id ()) {}
  hash_t to_hash () const { return ((_caller << 5) + _caller) ^ _callee; }
  bool operator== (const edge_key_t &b) const 
  { return _caller == b._caller && _callee == b._callee; }
  u_int32_t caller () const { return _caller; }
  u_int32_t callee () const { return _callee; }
private:
  const u_int32_t _caller;
  const u_int32_t _callee;
};

template<>
struct hashfn<edge_key_t>
{
  hashfn () {}
  hash_t operator() (const edge_key_t &in)  const { return in.to_hash (); }
};

template<>
struct equals<edge_key_t>
{
  equals () {}
  bool operator() (const edge_key_t &a, const edge_key_t &b) const
  { return a == b; }
};

//-----------------------------------------------------------------------

class edge_t {
public:
  edge_t (const edge_key_t &k) : _key (k), _hits (0) {}

  void inc () { _hits ++; }
  void report () const;

  edge_key_t _key;
  ihash_entry<edge_t> _lnk;
private:
  int _hits;
};

//-----------------------------------------------------------------------

class edge_table_t {
public:
  edge_table_t () {}
  edge_t *lookup (const call_site_t *caller, const call_site_t *callee);
  void reset () { _tab.deleteall (); }
  void report ();
  typedef ihash<edge_key_t, edge_t, &edge_t::_key, &edge_t::_lnk> tab_t;
private:
  tab_t _tab;
};

//-----------------------------------------------------------------------

class sfs_profiler_obj_t {
public:
  sfs_profiler_obj_t ();
  void enable ();
  void disable ();
  void reset ();
  void report ();
  void set_interval (time_t us);
  void set_real_timer (bool b);

  void timer_event (int sig, siginfo_t *si, void *uctx);

  enum { RANGE_SIZE_PCT = 10,
	 MIN_INTERVAL_US = 100,
	 DFLT_INTERVAL_US = 1000,
	 MAX_INTERVAL_US = 750000 };

  class trace_obj_t {
  public:
    trace_obj_t () {}
    vec<str> _trace;
  };

private:
  void mark_edge (call_site_t *b, call_site_t *t);
  call_site_t * lookup_pc (my_intptr_t pc);

  static time_t fix_interval (long in);
  void schedule_next_event ();
  void enable_timer (int which, const struct itimerval *itv);
  void disable_handler (int which);
  void enable_handler (int which);
  void disable_timer (int which);
  void crawl_stack (const ucontext_t &ut, trace_obj_t *to);
  void init_symbol_lookup ();
  
  time_t _interval_us;
  bool _enabled;
  bool _real;
  int _handler_typ;

  call_table_t _sites;
  edge_table_t _edges;

  bool _init;
  bool _is_static;
  void *_main_base;
  bool _running;
};

#define PRFX1 "SFS Profiler: "
#define PRFX2 "(SSP)"

static sfs_profiler_obj_t g_profile_obj;

//-----------------------------------------------------------------------

edge_t *
edge_table_t::lookup (const call_site_t *ee, const call_site_t *er)
{
  edge_key_t k (ee, er);
  edge_t *ret =  _tab[k];
  if (!ret) {
    ret = New edge_t (k);
    _tab.insert (ret);
  }
  return ret;
}

//-----------------------------------------------------------------------

call_site_t *
call_table_t::lookup (const char *file, my_intptr_t p, const void *base)
{
  obj_file_t *f = _files[file];
  if (!f) {
    f = New obj_file_t (file, base);
    _files.insert (f);
  }
  call_site_key_t k (f, p);
  call_site_t *s = _sites[k];
  if (!s) {
    s = New call_site_t (k);
    _sites.insert (s);
  }
  return s;
}

//-----------------------------------------------------------------------

sfs_profiler_obj_t::sfs_profiler_obj_t ()
  : _interval_us (DFLT_INTERVAL_US),
    _enabled (false),
    _real (false),
    _handler_typ (-1),
    _init (false),
    _is_static (false),
    _main_base (NULL),
    _running (false)
{
  srandom (time (NULL));
}

//-----------------------------------------------------------------------

time_t
sfs_profiler_obj_t::fix_interval (long in)
{
  return max<long> (MIN_INTERVAL_US, min<long> (in, MAX_INTERVAL_US));
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::schedule_next_event ()
{
  struct itimerval val;

  long rsz = _interval_us * RANGE_SIZE_PCT / 100;
  long offset = random () % rsz - (rsz/2);
  long interval = fix_interval (offset + _interval_us);

  val.it_value.tv_usec = interval;
  val.it_value.tv_sec = 0;
  val.it_interval.tv_usec = 0;
  val.it_interval.tv_sec = 0;

  int which = _real ? ITIMER_REAL : ITIMER_VIRTUAL;
  enable_timer (which, &val);
}

//-----------------------------------------------------------------------

static int
itimer2signal (int which)
{
  switch (which) {
  case ITIMER_REAL: return SIGALRM;
  case ITIMER_VIRTUAL: return SIGVTALRM;
  default: panic("bad timer value %d!!\n", which); break;
  }
  return -1;
}

//-----------------------------------------------------------------------

static void
s_timer_event (int sig, siginfo_t *si, void *uctx)
{
  g_profile_obj.timer_event (sig, si, uctx);
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::enable_handler (int which)
{
  if (_handler_typ != which) {
    if (_handler_typ >= 0) {
      disable_handler (_handler_typ);
    }
    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));
    sa.sa_sigaction = s_timer_event;
    sa.sa_flags =  SA_SIGINFO;
    sigaction (itimer2signal (which), &sa, NULL);
    _handler_typ = which;
  }
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::disable_handler (int which)
{
  disable_timer (which);
  sigaction (itimer2signal (which), NULL, NULL);
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::enable_timer (int which, const struct itimerval *itv)
{
  enable_handler (which);
  setitimer (which, itv, NULL);
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::disable_timer (int which)
{
  struct itimerval val;
  memset (&val, 0, sizeof (val));
  setitimer (which, &val, NULL);
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::enable ()
{
  warn << PRFX1 << "enabled\n";
  if (!_enabled) {
    schedule_next_event ();
    _enabled = true;
  }
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::disable ()
{
  warn << PRFX1 << "disabled\n";
  _enabled = false;
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::report ()
{
  _running = true;
  warn << PRFX2 << " ++++ start report ++++\n";
  _sites.report ();
  _edges.report ();
  warn << PRFX2 << " ---- end report ----\n";
  _running = false;
  if (_enabled) {
    schedule_next_event ();
  }
}

//-----------------------------------------------------------------------

void 
call_table_t::report ()
{
  report_files ();
  report_sites ();
}

//-----------------------------------------------------------------------

void 
call_table_t::report_files ()
{
  ihash_iterator_t<obj_file_t, file_tab_t> it (_files);
  const obj_file_t *p;
  while ((p = it.next ())) { p->report (); }
}

//-----------------------------------------------------------------------

void 
call_table_t::report_sites ()
{
  ihash_iterator_t<call_site_t, site_tab_t> it (_sites);
  const call_site_t *p;
  while ((p = it.next ())) { p->report (); }
}

//-----------------------------------------------------------------------

void
edge_t::report () const
{
  warn ("%s edge: %" PRId32 " %" PRId32 " %d\n",
	PRFX2, _key.caller (), _key.callee (), _hits);
}

//-----------------------------------------------------------------------

void 
obj_file_t::report () const
{
  warn ("%s file: %" PRId32 " %s %p %d\n",  PRFX2, _id, _name, _base, _main);
}

//-----------------------------------------------------------------------

void
edge_table_t::report ()
{
  ihash_iterator_t<edge_t, tab_t> it (_tab);
  const edge_t *p;
  while ((p = it.next ())) { p->report (); }
}

//-----------------------------------------------------------------------

void
call_site_t::report () const
{
  warn ("%s site: %" PRId32 " %" PRId32 " 0x%lx\n",  
	PRFX2, _id, _key.file ()->id (), _key.offset ());
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::reset ()
{
  warn << PRFX1 << "reset\n";
  _edges.reset ();
  _sites.reset ();
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::set_interval (time_t us)
{
  _interval_us = fix_interval (us);
  warn << PRFX1 << "set_interval to " << _interval_us << "us\n";
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::mark_edge (call_site_t *caller, call_site_t *callee)
{
  edge_t *e = _edges.lookup (caller, callee);
  if (e) e->inc ();
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::init_symbol_lookup ()
{
  if (!_init) {
    _init = true;
  }
}

//-----------------------------------------------------------------------

call_site_t *
sfs_profiler_obj_t::lookup_pc (my_intptr_t pc)
{
  call_site_t *ret = NULL;

  init_symbol_lookup ();

#if SFS_COMPILE_SHARED
  void *v = reinterpret_cast<void *> (pc);
  Dl_info info;
  memset (&info, 0, sizeof (info));
  int rc = dladdr (v, &info);
  if (rc == 1 && info.dli_fname) {
    const char *f = info.dli_fname;
    void *base = info.dli_fbase;
    ret = _sites.lookup (f, pc, base);
  }
#else
  ret = _sites.lookup (progname, pc, 0x0);
#endif

  return ret;
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::crawl_stack (const ucontext_t &ctx, trace_obj_t *to)
{
  
#ifdef UCONTEXT_RBP
  my_intptr_t top[1];
  const my_intptr_t *framep;
  framep = reinterpret_cast<const my_intptr_t *> (ctx.UCONTEXT_RBP);
  int lim = 1024;
  call_site_t *curr = NULL, *prev = NULL;
  call_site_t *last_good = NULL;

  while (!(reinterpret_cast<my_intptr_t> (framep) & 7) &&
	 framep > top &&
	 framep[0] && 
	 lim--) {

    my_intptr_t pc = framep[1] - 1;
    curr = lookup_pc (pc);
    if (curr) last_good = curr;
    if (curr && prev) { mark_edge (curr, prev); }
    prev = curr;

    framep = reinterpret_cast<const my_intptr_t *> (*framep);
  }

  if (last_good)  {
    last_good->set_as_main ();
  }


#endif /* UCONTEXT_RBP */
}

//-----------------------------------------------------------------------

void 
sfs_profiler_obj_t::timer_event (int sig, siginfo_t *si, void *uctx)
{
  if (_running) {
    return;
  }

  trace_obj_t to;
  if (sig == SIGALRM || sig == SIGVTALRM) {
    const ucontext_t *ut = static_cast<ucontext_t *> (uctx);
    crawl_stack (*ut, &to);
  }
  if (_enabled) {
    schedule_next_event ();
  }
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::set_real_timer (bool b)
{
  _real = b;
}

//-----------------------------------------------------------------------

void sfs_profiler::enable () { g_profile_obj.enable (); }
void sfs_profiler::disable () { g_profile_obj.disable (); }
void sfs_profiler::reset () { g_profile_obj.reset (); }
void sfs_profiler::report () { g_profile_obj.report (); }
void sfs_profiler::set_interval (time_t us) { g_profile_obj.set_interval (us); }
void sfs_profiler::set_real_timer (bool b) { g_profile_obj.set_real_timer (b); }

//-----------------------------------------------------------------------

#endif /* SIMPLE_PROFILER */
