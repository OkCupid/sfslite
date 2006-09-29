// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "corebench.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "litetime.h"

typedef enum { SFS_MODE = 0, cwait_MODE = 1, NONBLOCK_MODE = 2,
	       CHECK_CPUSPEED_MODE = 3,
	       BENCHMARK_WRAP_MODE = 4,
	       BENCHMARK_FUNCTOR_MODE = 5,
	       SFS_NULL =6 ,TAME_NULL =7,
	       BENCHMARK_SYSCALL = 8 } bmode_t;

int niter = 10000;
int ntimes = 10;
bmode_t mode;

class statobj_t {
public:
  statobj_t () : min (INT_MAX) {}
  void raw_result (u_int64_t raw) {
    if (raw < min) min = raw;
    raw_results.push_back (raw);
  }

  void add_result_set (double r) {
    results.push_back (r); 
  }

  double mean () {
    double f = 0;
    for (size_t i = 0; i < results.size (); i++) {
      f += results[i];
    }
    f = f / results.size ();
    return f;
  }

  double stddev () {
    double f = 0;
    double m = mean ();
    double n = results.size ();
    for (size_t i = 0; i < results.size (); i++) {
      double t = (results[i] - m);
      t = t *t;
      t = t / n;
      f += t;
    }
    f= sqrt (f);
    return f;
  }


  void report () {
    double m = mean ();
    double sd = stddev ();
    fprintf (stderr, 
	     "mean: %0.6g; sd: %0.6g, sd/mean: %0.6g%% min: %lld\n", 
	     m, sd, sd / m * 100, min);

    for (size_t s = 0; s < raw_results.size (); s++) {
      printf ("%lld\n", raw_results[s] );
    }
  }
  vec<double> results;
  vec<u_int64_t> raw_results;
  u_int64_t min;
};

#define REPORT(code) \
do { \
  statobj_t so; \
  { code } \
  { code } \
  for (int __j = 0; __j < ntimes; __j++) { \
    { code } \
    so.add_result_set (runtime); \
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
	   "%s: %lld %s; time/call=%0.6g %s\n", 
	   where, diff, tick_name, r, tick_name);
}

static void report2 (u_int64_t diff)
{
  u_int64_t diff2 = diff - time_in_acheck;
  report (diff);
  report (time_in_acheck, "in acheck");
  report (diff2, "not in acheck");
  //fprintf (stderr, "n wrap calls: %lld%\n", n_wrap_calls);
}

static void check_cpu_speed ()
{
  u_int64_t start = corebench_get_time ();
  sleep (1);
  u_int64_t end = corebench_get_time ();
  warn << end - start << " cycles/second\n";
}

static void wrap_me (int i, int j, int k, int l) {}

static void benchmark_wrap (bool dealloc)
{
  vec<callback<void, int, int>::ptr> v;
  v.setsize (niter);
  u_int64_t t , e, b;
  double runtime ;

  REPORT({
    t = 0;
    for (int i = 0; i < niter; i++) {
      b = corebench_get_time ();
      v[i] = wrap (wrap_me, 10, 20);
      if (dealloc) v[i] = NULL;
      e = corebench_get_time ();
      so.raw_result (e - b);
      t += (e-b);
    }
    runtime = t / (double)niter;
    for (int i = 0; i < niter; i++) { v[i] = NULL; }
  });
}


static void benchmark_syscall ()
{
  struct timeval tv;
  u_int64_t start, end, t;
  double runtime;

  REPORT({ 
    t = 0;
    for (int i = 0; i < niter; i++) {
      start = corebench_get_time ();
      gettimeofday (&tv, NULL);
      end = corebench_get_time ();
      so.raw_result (end - start);
      t += (end - start);
    }
    runtime = t / (double)niter;
  });
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

tamed static void foo () {}

static void bar ()
{
  ptr<closure_t> c (NULL);
  delaycb (0, 0, wrap (foo, c));
}

tamed static void
bench1_cwait (int niter, cbv done)
{
  cvars { int i; }
  for (i = 0; i < niter; i++) {
    cwait { delaycb (0, 0, mkevent()); }
  }
  (*done) ();
}

tamed static void
bench1_nulltame ()
{
  cvars { int i (0); }
  i++;
}

static void
bench1_nullsfs_alloc ()
{
  int *ip = New int ();
  (*ip) ++;
  delete ip;
}

static void
bench1_nullsfs ()
{
  int i = 0;
  i++;
}

static void bench1_null (bool alloc)
{
  u_int64_t b,e,t;
  double runtime;
  REPORT({
    t = 0;
    for (int i = 0; i < niter; i++) {
      switch (mode) {
      case SFS_NULL:
	if (alloc) {
	  b = corebench_get_time ();
	  bench1_nullsfs_alloc ();
	  e = corebench_get_time ();
	} else {
	  b = corebench_get_time ();
	  bench1_nullsfs ();
	  e = corebench_get_time ();
	}
	break;
      case TAME_NULL:
	b = corebench_get_time ();
	bench1_nulltame ();
	e = corebench_get_time ();
	break;
      default:
	panic ("whoops!");
      }
      so.raw_result (e - b);
      t += (e - b);
    }
    runtime = t / (double)niter;
  });
}

tamed static void
bench1_noncwait (int niter, cbv done)
{
  cvars {
    int i ;
    coordgroup_t<> G;
  }
  for (i = 0; i < niter; i++) {
    delaycb (0, 0, mkevent (G) );
    cwait (G);
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

tamed static void
harness ()
{
  cvars {
    int i (0), j (0);
    u_int64_t start, stop, d, tot (0);
    statobj_t so;
  }
  for (j = 0; j < 2; j++) {
    for (i = 0; i < (j == 0 ? 1 : ntimes); i++) {
      start = corebench_get_time ();
      cwait { 
	switch (mode) {
	case SFS_MODE:
	  bench1_sfs (niter, mkevent());
	  break;
	case cwait_MODE:
	  bench1_cwait (niter, mkevent());
	  break;
	case NONBLOCK_MODE:
	  bench1_noncwait (niter, mkevent());
	  break;
	default:
	  panic ("unknown mode");
	}
      }
      stop = corebench_get_time ();
      d = stop - start;
      if (j == 1) {
	tot += d;
	so.raw_result (d/niter);
	so.add_result_set (d / (double)niter);
      }
    }
  }
  so.report ();
  report2 (tot);
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

  while ((ch = getopt (argc, argv, "Nfwdcnbi:t:sa")) != -1) {
    switch (ch) {
    case 'a':
      do_alloc = true;
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
      mode = cwait_MODE;
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

  if (mode == CHECK_CPUSPEED_MODE) {
    warn << "Checking CPU speed...\n";

    // can't do this with a hardware timer running, since it will
    // interrupt the sleep(1) call!
    check_cpu_speed (); 
    exit (0);
  }
  sfs_set_clock (SFS_CLOCK_TIMER);

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
    panic ("wtf\n");
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
    warn << "using NONcwait mode for full test...\n";
    break;
  case cwait_MODE:
    if (null_test) {
      mode = TAME_NULL;
      warn << "doing a TAME NULL call test...\n";
      bench1_null (do_alloc);
      exit (0);
    } else {
      warn << "using cwait mode for full test...\n";
    }
    break;
  default:
    break;
  }
    
  harness ();
  amain ();
  bar ();
}
