// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "bench.h"

typedef enum { SFS_MODE = 0, BLOCK_MODE = 1, NONBLOCK_MODE = 2 } bmode_t;

int niter = 10000;
int ntimes = 100;
bmode_t mode;

static void usage ()
{
  fatal << progname << " [ -n | -b] [ -t <times>] [-i <n_iterations> ]\n";
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
    int i (0);
    u_int64_t start, stop;
  }
  start = get_time ();
  for (i = 0; i < ntimes; i++) {
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
  stop = get_time ();
  warn ("ntimes=%d; niter=%d; mode=%d: %lld usec\n", ntimes, niter, mode,
	stop - start);
  exit (0);
}
 
 
int main (int argc, char *argv[])
{
  int ch;
  setprogname (argv[0]);

  while ((ch = getopt (argc, argv, "nbi:t:")) != -1) {
    switch (ch) {
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
  harness ();
  amain ();
  bar ();
}
