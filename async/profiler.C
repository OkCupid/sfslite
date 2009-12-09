
#include "sysconf.h"
#include "sfs_profiler.h"

//-----------------------------------------------------------------------

#ifdef SIMPLE_PROFILER

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
#include <ucontext.h>

#include <dlfcn.h>
#include "ihash.h"
#include <setjmp.h>

#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>

//-----------------------------------------------------------------------

#define ENTER_PROFILER()  bool __r = _running; _running = true
#define EXIT_PROFILER() _running = __r;

//-----------------------------------------------------------------------

enum { COLS_PER_LINE = 8 };

//-----------------------------------------------------------------------

typedef u_long my_intptr_t;

//-----------------------------------------------------------------------

#ifdef HAVE_LIBUNWIND

typedef struct {
  void **result;
  int max_depth;
  int skip_count;
  int count;
} trace_arg_t;

typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8
} my_unwind_reason_code_t;

struct _Unwind_Context;

extern "C" {
  
  void _Unwind_Backtrace (int (*fn) (struct _Unwind_Context *, void *),
			  trace_arg_t *arg);
  my_intptr_t _Unwind_GetIP(struct _Unwind_Context *ctx);
};

#endif

//-----------------------------------------------------------------------

typedef enum { REPORT_NONE = 0,
	       REPORT_SITE = 1,
	       REPORT_EDGE = 2 } report_type_t;

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

  void reset () { _sites.clear (); _files.clear (); }
  void report ();
  void report_sites ();
  void report_files ();
  void recharge ();

  enum { MIN_ROOM = 0x1000, MIN_SIZE = 0x10000 };

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

  enum { MIN_ROOM = 0x1000, MIN_SIZE = 0x10000 };

  void reset () { _tab.clear (); }
  void report ();
  void recharge ();
  typedef ihash<edge_key_t, edge_t, &edge_t::_key, &edge_t::_lnk> tab_t;
private:
  tab_t _tab;
};

//-----------------------------------------------------------------------

class sfs_profiler_obj_t {
public:
  sfs_profiler_obj_t ();
  bool enable ();
  void disable ();
  void reset ();
  void report ();
  void set_interval (time_t us);
  void set_real_timer (bool b);

  void timer_event (int sig, siginfo_t *si, void *uctx);
  void recharge ();
  inline void enter_vomit_lib ();
  void exit_vomit_lib ();
  inline void init ();

  enum { RANGE_SIZE_PCT   = 10,
	 MIN_INTERVAL_US  = 100,
	 DFLT_INTERVAL_US = 5000,
	 MAX_INTERVAL_US  = 750000 };

  edge_t *alloc_edge (const edge_key_t &k);
  obj_file_t *alloc_file (const char *file, const void *base);
  call_site_t *alloc_site (const call_site_key_t &k);

  enum { MIN_SCRATCH_SIZE = 0x10000,
	 DEF_SCRATCH_SIZE = 0x100000 };

private:
  void mark_edge (call_site_t *b, call_site_t *t);
  call_site_t * lookup_pc (my_intptr_t pc);

  static time_t fix_interval (long in);
  void schedule_next_event ();
  void enable_timer (int which, const struct itimerval *itv);
  void disable_handler (int which);
  void enable_handler (int which);
  void disable_timer (int which);
  void crawl_stack (const ucontext_t &ut);
  void buffer_recharge ();
  bool valid_rbp (const my_intptr_t *i) const;
  bool valid_rbp_strict (const my_intptr_t *i, const my_intptr_t *s) const;
  const my_intptr_t *stack_step (const my_intptr_t *i) const;

  time_t _interval_us;
  bool _enabled;
  bool _real;
  int _handler_typ;

  call_table_t _sites;
  edge_table_t _edges;

  bool _init;
  bool _is_static;
  void *_main_base;
  const my_intptr_t *_vomit_rbp;
  bool _running;

  vec<char *> _bufs;
  char *_buf, *_bp, *_endp;

  const my_intptr_t *_main_rbp;
};

#define PRFX1 "SFS Profiler: "
#define PRFX2 "(SSP)"

static sfs_profiler_obj_t g_profile_obj;

//-----------------------------------------------------------------------

template<class T> static void
ihash_recharge (T &tab, size_t min_size, size_t min_room) 
{
  if (tab.size () == 0) {
    tab.reserve (min_size);
  } else if (tab.room () < ssize_t (min_room)) {
    tab.grow ();
  }
}

//-----------------------------------------------------------------------

void
call_table_t::recharge ()
{
  ihash_recharge (_files, MIN_SIZE, MIN_ROOM);
  ihash_recharge (_sites, MIN_SIZE, MIN_ROOM);
}

//-----------------------------------------------------------------------

void
edge_table_t::recharge ()
{
  ihash_recharge (_tab, MIN_SIZE, MIN_ROOM);
}

//-----------------------------------------------------------------------

edge_t *
edge_table_t::lookup (const call_site_t *ee, const call_site_t *er)
{
  edge_key_t k (ee, er);
  edge_t *ret =  _tab[k];
  if (!ret) {
    if (_tab.has_room ()) {
      ret = g_profile_obj.alloc_edge (k);
      if (ret) {
	_tab.insert (ret);
      }
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

call_site_t *
call_table_t::lookup (const char *file, my_intptr_t p, const void *base)
{
  obj_file_t *f = _files[file];
  call_site_t *s = NULL;
  if (!f) {
    if (_files.has_room ()) {
      f = g_profile_obj.alloc_file (file, base);
      if (f) {
	_files.insert (f);
      }
    }
  }
  if (f) {
    call_site_key_t k (f, p);
    s = _sites[k];
    if (!s) {
      if (_sites.has_room ()) {
	s = g_profile_obj.alloc_site (k);
	if (s) {
	  _sites.insert (s);
	}
      }
    }
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
    _vomit_rbp (NULL),
    _running (false),
    _buf (NULL),
    _bp (NULL),
    _endp (NULL),
    _main_rbp (NULL)
{
  srandom (time (NULL));
}

//-----------------------------------------------------------------------

bool
sfs_profiler_obj_t::valid_rbp (const my_intptr_t *rbp) const
{
  return (rbp &&
	  !(reinterpret_cast<my_intptr_t> (rbp) & 7) &&
	  rbp[0]);
}

//-----------------------------------------------------------------------

bool
sfs_profiler_obj_t::valid_rbp_strict (const my_intptr_t *rbp,
				      const my_intptr_t *sig) const
{
  // Important to check valid_rbp last!!
  return (_main_rbp &&
	  rbp <= _main_rbp &&
	  rbp >= sig &&
	  valid_rbp (rbp));
}

//-----------------------------------------------------------------------

const my_intptr_t *
sfs_profiler_obj_t::stack_step (const my_intptr_t *r) const
{
  return reinterpret_cast<const my_intptr_t *> (*r);
}

//-----------------------------------------------------------------------

#ifdef RBP_ASM_REG

# define READ_RBP(x)						\
  __asm volatile ( RBP_ASM_REG ", %0" : "=g" (x) :)

#else
# ifdef UCONTEXT_RBP

# define READ_RBP(x)							\
  do {									\
   ucontext_t uc;							\
   getcontext (&uc);							\
    x = reinterpret_cast<const my_intptr_t *> (uc.UCONTEXT_RBP);	\
 } while (0)

# else
# define READ_RBP(x)				\
  x = NULL

# endif
#endif

//-----------------------------------------------------------------------

#define GOTO_FRAME(ret,steps)					\
  do {								\
    READ_RBP(ret);						\
    for (size_t i = 0; valid_rbp (ret) && i < steps; i++) {	\
      ret = stack_step (ret);					\
    }								\
  } while (0)

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::enter_vomit_lib ()
{
  if (_enabled) {
    _vomit_rbp = NULL;
    GOTO_FRAME(_vomit_rbp, 1);
  }
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::exit_vomit_lib ()
{
  if (_enabled) {
    _vomit_rbp = NULL;
  }
}

//-----------------------------------------------------------------------

static my_intptr_t 
framep2pc (const my_intptr_t *framep)
{
  return framep[1] - 1;
}

//-----------------------------------------------------------------------

// Should be called from main or amain
void
sfs_profiler_obj_t::init ()
{
  ENTER_PROFILER ();
  if (!_init) {
  
    const my_intptr_t *framep;
    GOTO_FRAME(framep, 1);
    
    if (!(_main_rbp = framep)) {
      warn ("Cannot initialize profiler: cannot find main stack!\n");
    }
    _init = true;

    recharge ();
  }
  EXIT_PROFILER ();
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::buffer_recharge ()
{
  if (!_buf || (_endp - _bp) < MIN_SCRATCH_SIZE) {
    if (_buf) {
      _bufs.push_back (_buf);
    }
    _bp = _buf = New char[DEF_SCRATCH_SIZE];
    _endp = _bp + DEF_SCRATCH_SIZE;
  }
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::recharge ()
{
  ENTER_PROFILER();
  if (_enabled && _init) {
    buffer_recharge ();
    _sites.recharge ();
    _edges.recharge ();
  }
  EXIT_PROFILER();
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

bool
sfs_profiler_obj_t::enable ()
{
  ENTER_PROFILER();
  warn << PRFX1 << "enabled\n";
  if (!_enabled) {
    _enabled = true;
  }
  recharge ();
  schedule_next_event ();
  EXIT_PROFILER();
  return true;
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
  ENTER_PROFILER();
  warn << PRFX2 << " ++++ start report ++++\n";
  _sites.report ();
  _edges.report ();
  warn << PRFX2 << " ---- end report ----\n";
  if (_enabled) {
    schedule_next_event ();
  }
  EXIT_PROFILER();
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

static void
col_format_pre (size_t n, size_t line_size, const char *prfx)
{
  if ((n % line_size) == 0) {
    warn << PRFX2 << " " << prfx << ": ";
  } else {
    warnx << "; ";
  }
}

//-----------------------------------------------------------------------

static void
col_format_post (size_t n, size_t line_size)
{
  if ((n + 1) % line_size == 0) {
    warnx << "\n";
  }
}

//-----------------------------------------------------------------------

static void
col_format_final (size_t n, size_t line_size)
{
  if (n > 0 && (n % line_size != 0)) {
    warnx << "\n";
  }
}

//-----------------------------------------------------------------------

void 
call_table_t::report_sites ()
{
  ihash_iterator_t<call_site_t, site_tab_t> it (_sites);
  const call_site_t *p;
  size_t col = 0;
  size_t cols = COLS_PER_LINE;

  while ((p = it.next ())) { 
    col_format_pre (col, cols, "s");
    p->report ();
    col_format_post (col, cols);
    col++;
  }
  col_format_final (col, cols);
}

//-----------------------------------------------------------------------

void
edge_t::report () const
{
  warnx ("%" PRId32 " %" PRId32 " %d",
	 _key.caller (), _key.callee (), _hits);
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
  size_t col = 0;
  size_t cols = COLS_PER_LINE;

  while ((p = it.next ())) { 
    col_format_pre (col, cols, "e");
    p->report ();
    col_format_post (col, cols);
    col++;
  }
  col_format_final (col, cols);
}

//-----------------------------------------------------------------------

void
call_site_t::report () const
{
  warnx ("%" PRId32 " %" PRId32 " 0x%lx",  
	_id, _key.file ()->id (), _key.offset ());
}

//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::reset ()
{
  warn << PRFX1 << "reset\n";

  // Don't allow the profile to run while we're resetting it
  bool e = _enabled;
  if (e) { disable (); }
  _edges.reset ();
  _sites.reset ();

  if (_buf) {
    delete [] _buf;
    _buf = _bp = _endp = NULL;
  }
  while (_bufs.size ()) { delete [] _bufs.pop_back (); }
  if (e) { enable (); }
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

call_site_t *
sfs_profiler_obj_t::lookup_pc (my_intptr_t pc)
{
  call_site_t *ret = NULL;

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

#ifdef HAVE_LIBUNWIND
static int
get_one_frame (struct _Unwind_Context *uc, void *opq)
{
 trace_arg_t *targ = (trace_arg_t *) opq;

  if (targ->skip_count > 0) {
    targ->skip_count--;
  } else {
    targ->result[targ->count++] = (void *) _Unwind_GetIP(uc);
  }

  if (targ->count == targ->max_depth)
    return _URC_END_OF_STACK;
  return 0;
}
#endif /* HAVE_LIBUNWIND */ 

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

void
sfs_profiler_obj_t::crawl_stack (const ucontext_t &ctx)
{
  call_site_t *curr = NULL, *prev = NULL;
  call_site_t *last_good = NULL;

#define DEPTH 256
#ifdef HAVE_LIBUNWIND
  void *result[DEPTH];
  trace_arg_t targ;
  targ.result = result;
  targ.max_depth = DEPTH;
  targ.skip_count = 3;
  targ.count = 0;
  
  _Unwind_Backtrace (get_one_frame, &targ);
  
  for (int i = 0; i < targ.count; i++) {
    my_intptr_t pc = reinterpret_cast<my_intptr_t> (result[0]);
    curr = lookup_pc (pc);
    if (curr) last_good = curr;
    if (curr && prev) { mark_edge (curr, prev); }
    prev = curr;
  }

#else
# ifdef UCONTEXT_RBP
  const my_intptr_t *framep, *sigstack;

  if (!(framep = _vomit_rbp)) {
    framep = reinterpret_cast<const my_intptr_t *> (ctx.UCONTEXT_RBP);
  }
  READ_RBP(sigstack);

  int lim = DEPTH;

  while (valid_rbp_strict (framep, sigstack) && lim--) {

    my_intptr_t pc = framep2pc (framep);
    curr = lookup_pc (pc);
    if (curr) last_good = curr;
    if (curr && prev) { mark_edge (curr, prev); }
    prev = curr;

    framep = stack_step (framep);
  }

# endif /* UCONTEXT_RBP */
#endif /* HAVE_LIBUNWIND */

  if (last_good)  {
    last_good->set_as_main ();
  }
#undef DEPTH
}

//-----------------------------------------------------------------------

void 
sfs_profiler_obj_t::timer_event (int sig, siginfo_t *si, void *uctx)
{
  if (!_running && _init && _enabled) {
    if (sig == SIGALRM || sig == SIGVTALRM) {
      const ucontext_t *ut = static_cast<ucontext_t *> (uctx);
      crawl_stack (*ut);
    }
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

call_site_t *
sfs_profiler_obj_t::alloc_site (const call_site_key_t &k)
{
  call_site_t *ret = NULL;
  size_t sz = sizeof (call_site_t);
  if (_buf && _bp + sz <= _endp) {
    void *v = _bp;
    ret = new (v) call_site_t (k);
    _bp += sz;
  }
  return ret;
}

//-----------------------------------------------------------------------

obj_file_t *
sfs_profiler_obj_t::alloc_file (const char *f, const void *o)
{
  obj_file_t *ret = NULL;
  size_t sz = sizeof (obj_file_t);
  if (_buf && _bp + sz < _endp) {
    void *v = _bp;
    ret = new (v) obj_file_t (f, o);
    _bp += sz;
  }
  return ret;
}

//-----------------------------------------------------------------------

edge_t *
sfs_profiler_obj_t::alloc_edge (const edge_key_t &k)
{
  edge_t *ret = NULL;
  size_t sz = sizeof (edge_t);
  if (_buf && _bp + sz < _endp) {
    void *v = _bp;
    ret = new (v) edge_t (k);
    _bp += sz;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool sfs_profiler::enable () { return g_profile_obj.enable (); }
void sfs_profiler::disable () { g_profile_obj.disable (); }
void sfs_profiler::reset () { g_profile_obj.reset (); }
void sfs_profiler::report () { g_profile_obj.report (); }
void sfs_profiler::set_interval (time_t us) { g_profile_obj.set_interval (us); }
void sfs_profiler::set_real_timer (bool b) { g_profile_obj.set_real_timer (b); }
void sfs_profiler::recharge () { g_profile_obj.recharge (); }
void sfs_profiler::enter_vomit_lib () { g_profile_obj.enter_vomit_lib (); }
void sfs_profiler::exit_vomit_lib () { g_profile_obj.exit_vomit_lib (); }
void sfs_profiler::init () { g_profile_obj.init (); }

//-----------------------------------------------------------------------

namespace sfs {

  //-----------------------------------------------------------------------

  void *
  memcpy_p (void *dest, const void *src, size_t n)
  {
    sfs_profiler::enter_vomit_lib ();
    void *r = ::memcpy (dest, src, n);
    sfs_profiler::exit_vomit_lib ();
    return r;
  }

  //-----------------------------------------------------------------------

};


#else /* SIMPLE_PROFILER */

//-----------------------------------------------------------------------

bool sfs_profiler::enable () { return false; }
void sfs_profiler::disable () {}
void sfs_profiler::reset () {}
void sfs_profiler::report () {}
void sfs_profiler::set_interval (time_t us) {}
void sfs_profiler::set_real_timer (bool b) {}
void sfs_profiler::recharge () {}
void sfs_profiler::enter_vomit_lib () {}
void sfs_profiler::exit_vomit_lib () {}
void sfs_profiler::init () {}

//-----------------------------------------------------------------------

namespace sfs {

  //-----------------------------------------------------------------------

  void *
  memcpy_p (void *dest, const void *src, size_t n)
  {
    void *r = ::memcpy (dest, src, n);
    return r;
  }

  //-----------------------------------------------------------------------

};

//-----------------------------------------------------------------------

#endif /* SIMPLE_PROFILER */
