
#include "litetime.h"
#include "async.h"
#include <sys/time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

//-----------------------------------------------------------------------
// Begin Global Clock State
//
bool timer_enabled = false;
sfs_clock_t sfs_clock = SFS_CLOCK_GETTIME;

struct mmap_clock_t {
  mmap_clock_t (const str &fn) 
    : mmp (NULL), nbad (0), fd (-1), file (fn), 
    mmp_sz (sizeof (struct timespec) * 2)
  {
    ::clock_gettime (CLOCK_REALTIME, &last);
  }
  ~mmap_clock_t () ;
  bool init ();
  int clock_gettime (struct timespec *ts);

  enum { stale_threshhold = 50000 // number of stale values before we give up
  };

  struct timespec *mmp; // mmap'ed pointer
  int nbad;             // number of calls since diff
  struct timespec last; // last returned value
  int fd;               // fd for the file
  const str file;       // file name of the mmaped clock file
  const size_t mmp_sz;  // size of the mmaped region
  
};
mmap_clock_t *mmap_clock;

//
// End Global Clock State
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// Mmap Clock Type
//

static bool
enable_mmap_clock (const str &arg)
{
  if (mmap_clock) 
    return true;

  mmap_clock = New mmap_clock_t (arg);
  return mmap_clock->init ();
}

static void
disable_mmap_clock ()
{
  if (mmap_clock) {
    delete mmap_clock;
    mmap_clock = NULL;
  }
}

void
mmap_clock_fail ()
{
  warn << "*mmap clock failed: reverting to stable clock\n";
  disable_mmap_clock ();
  sfs_clock = SFS_CLOCK_GETTIME;
}


mmap_clock_t::~mmap_clock_t ()
{
  if (mmp) 
    munmap (mmp, mmp_sz);
  if (fd >= 0)
    close (fd);
}

bool
mmap_clock_t::init ()
{
  void *tmp;
  struct stat sb;

  if ((fd = open (file.cstr (), O_RDONLY)) < 0) {
    warn ("%s: mmap clock file open failed: %m\n", file.cstr ());
    return false;
  }

  if (fstat (fd, &sb) < 0) {
    warn ("%s: cannot fstat file: %m\n", file.cstr ());
    return false;
  }
  if (sb.st_size < mmp_sz) {
    warn << file << ": short file; aborting\n";
    return false; 
  }

  tmp = mmap (NULL, mmp_sz, PROT_READ, MAP_NOSYNC|MAP_SHARED, fd, 0); 
  if (tmp == MAP_FAILED) {
    warn ("%s: mmap clock mmap failed: %m\n", file.cstr ());
    return false;
  }
  mmp = (struct timespec *)tmp;
  warn << "*unstable: mmap clock initialized\n";
  return true;
}

int
mmap_clock_t::clock_gettime (struct timespec *out)
{
  struct timespec tmp;

  assert (mmap_clock);

  *out = mmp[0];
  tmp = mmp[1];

  // either we're unlucky or the guy crashed an a strange state
  // which is unlucky too
  if (!TIMESPEC_EQ (*out, tmp)) {
    
    // debug message
    //warn << "*mmap clock: reverting to clock_gettime\n";

    ::clock_gettime (CLOCK_REALTIME, out);
    last = *out;
    ++nbad;

    //
    // likely case -- timestamp in shared memory is stale.
    //
  } else if (TIMESPEC_LT (*out, last)) {
    TIMESPEC_INC (&last) ;
    *out = last;
    ++nbad;

    //
    // if the two stamps are equal, and they're strictly greater than
    // any timestamp we've previously issued, then it's OK to use it
    // as normal. this should be happening every so often..
    //
  } else {
    last = *out;
    nbad = 0;
  }

  if (nbad > stale_threshhold) 
    // will delete this, so be careful
    mmap_clock_fail ();

  return 0;
}


//
// End of Mmap clock type
//-----------------------------------------------------------------------




//-----------------------------------------------------------------------
// TIMER clock type
//
static void 
clock_set_timer ()
{
  struct itimerval val;
  val.it_value.tv_sec = 0;
  val.it_value.tv_usec = 10000; // 10 milliseconds
  val.it_interval.tv_sec = 0;
  val.it_interval.tv_usec = 10000; // 10 milliseconds

  setitimer (ITIMER_REAL, &val, 0);
}

static void
clock_timer_event ()
{
  clock_gettime (CLOCK_REALTIME, &tsnow);
}

static bool
enable_timer ()
{
  if (!timer_enabled) {
    warn << "*unstable: enabling hardware timer\n";
    clock_timer_event ();
    timer_enabled = true;
    sigcb (SIGALRM, wrap (clock_timer_event));
    clock_set_timer ();
  }
  return true;
}

static void
disable_timer ()
{
  if (timer_enabled) {
    warn << "disabling timer\n";
    struct itimerval val;
    memset (&val, 0, sizeof (val));
    setitimer (ITIMER_REAL, &val, 0);
    timer_enabled = false;
  }
}
//
// End TIMER clock type
//-----------------------------------------------------------------------


//
//-----------------------------------------------------------------------
//
// main, publically available functions..
//

//
// my_clock_gettime
//
//    Function that is called many times through the event loop, and
//    thus, we might want to optimize it for our needes.
//
int
my_clock_gettime (struct timespec *tp)
{
  int r = 0;
  switch (sfs_clock) {
  case SFS_CLOCK_GETTIME:
    r = clock_gettime (CLOCK_REALTIME, tp);
    break;
  case SFS_CLOCK_TIMER:
    tsnow.tv_nsec ++;
    *tp = tsnow;
    break;
  case SFS_CLOCK_MMAP:
    r = mmap_clock->clock_gettime (tp);
    break;
  default:
    break;
  }
  return r;
}

//
// set_sfs_clock
//
//    Set the core timing discipline for SFS.
//
void
sfs_set_clock (sfs_clock_t typ, const str &arg)
{
  switch (typ) {
  case SFS_CLOCK_TIMER:
    disable_mmap_clock ();
    sfs_clock = enable_timer () ? SFS_CLOCK_TIMER : SFS_CLOCK_GETTIME;
    break;
  case SFS_CLOCK_MMAP:
    disable_timer ();
    if (enable_mmap_clock (arg))
      sfs_clock = typ;
    else
      mmap_clock_fail ();
    break;
  case SFS_CLOCK_GETTIME:
    disable_timer ();
    disable_mmap_clock ();
    sfs_clock = typ;
    break;
  default:
    assert (false);
  }
}
