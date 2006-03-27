// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "corebench.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

typedef enum { SFS_MODE = 0, BLOCK_MODE = 1, NONBLOCK_MODE = 2,
	       CHECK_CPUSPEED_MODE = 3,
	       BENCHMARK_WRAP_MODE = 4,
	       BENCHMARK_FUNCTOR_MODE = 5,
	       SFS_NULL =6 ,TAME_NULL =7,
	       BENCHMARK_SYSCALL = 8 } bmode_t;

int niter = 10000;
int ntimes = 100;
bmode_t mode;
bool report_paper_mode;

class statobj_t {
public:
  statobj_t () : min (INT_MAX) {}
  void add_result (u_int64_t raw, int n) { 
    float x = raw / (float )n;
    results.push_back (x);  
    if (raw < min) min = raw;
  }

  float mean () {
    float f = 0;
    for (size_t i = 0; i < results.size (); i++) {
      f += results[i];
    }
    f = f / results.size ();
    return f;
  }

  float stddev () {
    float f = 0;
    float m = mean ();
    for (size_t i = 0; i < results.size (); i++) {
      float t = (results[i] - m) * (results[i] - m);
      f += t;
    }
    f = f / (float)results.size ();
    return sqrtf (f);
  }


  void report () {
    float m = mean ();
    float sd = stddev ();
    fprintf (stderr, 
	     "mean: %0.6f; stddev: %0.6f; sttdev/mean: %0.6f%%; min: %lld\n", 
	     m, sd, sd/m*100, min);
  }
  vec<float> results;
  u_int64_t min;
};

#define REPORT(code,clear) \
do { \
  u_int64_t b,e; \
  statobj_t so; \
  { code }      \
  { code }      \
  { clear}      \
  for (int __i = 0; __i < ntimes; __i++) { \
    b = corebench_get_time (); \
    { code } \
    e = corebench_get_time ();  \
    so.add_result (e - b, niter); \
    { clear } \
   } \
   so.report (); \
} while (0) \



static void usage ()
{
  fatal << progname << " [ -n | -b] [ -t <times>] [-i <n_iterations> ]\n";
}

static void report (u_int64_t diff, const char *where= NULL)
{
  const char *tick_name = BENCH_TICK_TYPE;
  double r = (double)diff / ( (double) ntimes * niter);
  if (where == NULL) where = "total   ";
  fprintf (stderr, 
	   "%s: ntimes=%d; niter=%d; mode=%d: %lld %s; time/call=%0.6g %s\n", 
	   where, ntimes, niter, mode, diff, tick_name, r, tick_name);
}

static void report2 (u_int64_t diff)
{
  u_int64_t diff2 = diff - time_in_acheck;
  report (diff);
  report (time_in_acheck, "in acheck");
  report (diff2, "not in acheck");
  fprintf (stderr, "n wrap calls: %lld%\n", n_wrap_calls);
}

static void check_cpu_speed ()
{
  u_int64_t start = corebench_get_time ();
  sleep (1);
  u_int64_t end = corebench_get_time ();
  report (end - start);
}

static void wrap_me (int i, int j, int k, int l) {}

static void benchmark_wrap (bool dealloc)
{
  u_int64_t start, end;
  vec<callback<void,int,int>::ptr> v;
  v.setsize (niter);

  if (report_paper_mode) {
    REPORT(
	   for (int i = 0; i < niter; i++) {
	     v[i] = wrap (wrap_me, 10, 20);
	     if (dealloc) v[i] = NULL;
	   },
	   for (int i = 0; i < niter; i++) {
	     v[i] = NULL; 
	   });
  } else {
    ntimes = 1;
    for (int i = 0; i < niter; i++) {
      v[i] = wrap (wrap_me, 10, 20);
      v[i] = NULL;
    }
    
    start = corebench_get_time ();
    for (int i = 0; i < niter; i++) {
      v[i] = wrap (wrap_me, 10, 20);
      if (dealloc) v[i] = NULL;
    }
    end = corebench_get_time ();
    report (end - start);
  }
}


static void benchmark_syscall ()
{
  struct timeval tv;
  u_int64_t start, end;

  if (report_paper_mode) {
    REPORT( for (int i = 0; i < niter; i++) { gettimeofday (&tv, NULL); }, ; );
  } else {
    ntimes = 1;
    start = corebench_get_time ();
    for (int i = 0; i < niter; i++) {
      gettimeofday (&tv, NULL);
    }
    end = corebench_get_time ();
    report (end - start);
  }
}

static void benchmark_functor ()
{
  callback<void, int, int>::ptr cb = wrap (wrap_me, 10, 20);
  u_int64_t start, end;

  for (int i = 0; i < niter; i++) {
    (*cb)(30,40);
  }
  start = corebench_get_time ();
  for (int i = 0; i < niter; i++) {
    (*cb)(30,40);
  }
  end = corebench_get_time ();
  report (end - start);
}

TAMED static void foo () {}

static void bar ()
{
  ptr<closure_t> c (NULL);
  delaycb (0, 0, wrap (foo, c));
}

TAMED static void
bench1_block (int niter, cbv done)
{
  VARS { int i; }
  for (i = 0; i < niter; i++) {
    BLOCK { delaycb (0, 0, @()); }
  }
  (*done) ();
}

TAMED static void
bench1_nulltame ()
{
  VARS { int i (0); }
  i++;
}

static void
bench1_nullsfs (bool alloc)
{
  int *ip = NULL; 
  if (alloc) New int ();
  int i;
  i++;
  if (ip) delete ip;
}

static void bench1_null (bool alloc)
{
  if (mode == SFS_NULL) {
    REPORT (for (int i = 0; i < niter; i++) { bench1_nullsfs (alloc); } , ;);
  } else if (mode = TAME_NULL) {
    REPORT (for (int i = 0; i < niter; i++) { bench1_nulltame (); }, ;);
  }
}

TAMED static void
bench1_nonblock (int niter, cbv done)
{
  VARS {
    int i ;
    coordgroup_t<> G;
  }
  for (i = 0; i < niter; i++) {
    delaycb (0, 0, @[G]() );
    WAIT (G);
  }
  (*done) ();
}

static void
bench1_sfs2 (ptr<int> i, cbv done)
{
  if (*i <= 0) {
    (*done) ();
  } else {
    (*i) --;
    delaycb (0, 0, wrap (bench1_sfs2, i, done));
  }
}

void
bench1_sfs (int niter, cbv done)
{
  ptr<int> i = New refcounted<int> (niter);
  bench1_sfs2 (i, done);
}

TAMED static void
harness ()
{
  VARS {
    int i (0), j (0);
    u_int64_t start, stop;
  }
  for (j = 0; j < 2; j++) {
    start = corebench_get_time ();
    for (i = 0; i < (j == 0 ? 1 : ntimes); i++) {
      BLOCK { 
	switch (mode) {
	case SFS_MODE:
	  bench1_sfs (niter, @());
	  break;
	case BLOCK_MODE:
	  bench1_block (niter, @());
	  break;
	case NONBLOCK_MODE:
	  bench1_nonblock (niter, @());
	  break;
	default:
	  panic ("unknown mode");
	}
      }
    }
    stop = corebench_get_time ();
  }
  u_int64_t diff = stop - start;
  report2 (diff);
  exit (0);
}
 
 
int main (int argc, char *argv[])
{
  int ch;
  setprogname (argv[0]);
  bool dealloc (false);
  bool null_test (false);
  bool do_alloc = false;

  toggle_corebench (true);

  report_paper_mode = false;

  while ((ch = getopt (argc, argv, "RNfwdcnbi:t:sa")) != -1) {
    switch (ch) {
    case 'a':
      do_alloc = true;
      break;
    case 'R':
      report_paper_mode = true;
      break;
    case 'N':
      null_test = true;
      break;
    case 's':
      if (mode != SFS_MODE) usage ();
      mode = BENCHMARK_SYSCALL;
      break;
    case 'w':
      if (mode != SFS_MODE)
	usage ();
      mode = BENCHMARK_WRAP_MODE;
      break;
    case 'f':
      if (mode != SFS_MODE) usage ();
      mode = BENCHMARK_FUNCTOR_MODE;
      break;
    case 'd':
      dealloc = true;
      break;
    case 'c':
      if (mode != SFS_MODE) 
	usage ();
      mode = CHECK_CPUSPEED_MODE;
      break;
    case 't':
      if (!convertint (optarg, &ntimes))
	usage ();
      break;
    case 'i':
      if (!convertint (optarg, &niter))
	usage ();
      break;
    case 'b':
      if (mode != SFS_MODE) 
	usage ();
      mode = BLOCK_MODE;
      break;
    case 'n':
      if (mode != SFS_MODE)
	usage ();
      mode = NONBLOCK_MODE;
      break;
    default:
      usage ();
    }
  }

  switch (mode) {
  case BENCHMARK_SYSCALL:
    warn << "benchmarking syscall time...\n";
    benchmark_syscall ();
    exit (0);
    break;
  case BENCHMARK_FUNCTOR_MODE:
    warn << "Checking time to call a callback...\n";
    ntimes = 1;
    benchmark_functor ();
    exit (0);
    break;
  case CHECK_CPUSPEED_MODE:
    warn << "Checking CPU speed...\n";
    ntimes = 1;
    niter = 1;
    check_cpu_speed ();
    exit (0);
    break;
  case BENCHMARK_WRAP_MODE:
    warn << "Benchmarking wrap";
    if (dealloc)
      warnx << " (with deallocation)";
    warnx << "...\n";
    benchmark_wrap (dealloc);
    exit (0);
    break;
  case SFS_MODE:
    if (null_test) {
      mode = SFS_NULL;
      warn << "doing SFS NULL call test....\n";
      bench1_null (do_alloc);
      exit (0);
    } else {
      warn << "using SFS code for full test...\n";
    }
    break;
  case NONBLOCK_MODE:
    warn << "using NONBLOCK mode for full test...\n";
    break;
  case BLOCK_MODE:
    if (null_test) {
      mode = TAME_NULL;
      warn << "doing a TAME NULL call test...\n";
      bench1_null (do_alloc);
      exit (0);
    } else {
      warn << "using BLOCK mode for full test...\n";
    }
    break;
  default:
    break;
  }
    
  harness ();
  amain ();
  bar ();
}
