/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "async.h"
#include "fdlim.h"
#include "ihash.h"
#include "itree.h"
#include "list.h"

#include <typeinfo>

#define FD_SETSIZE_ROUND (sizeof (long))/* # Bytes to which to round fd_sets */
int fd_set_bytes;		// Size in bytes of a [wide] fd_set
int maxfd;
bool amain_panic;
static int nselfd;

timespec tsnow;
const time_t &timenow = tsnow.tv_sec;

static timeval selwait;

#ifdef WRAP_DEBUG
#define CBTR_FD    0x0001
#define CBTR_TIME  0x0002
#define CBTR_SIG   0x0004
#define CBTR_CHLD  0x0008
#define CBTR_LAZY  0x0010
static int callback_trace;
static bool callback_time;

static inline const char *
timestring ()
{
  if (!callback_time)
    return "";

  my_clock_gettime (&tsnow);
  static str buf;
  buf = strbuf ("%d.%06d ", int (tsnow.tv_sec), int (tsnow.tv_nsec/1000));
  return buf;
}
#endif /* WRAP_DEBUG */

struct child {
  pid_t pid;
  cbi cb;
  ihash_entry<child> link;
  child (pid_t p, cbi c) : pid (p), cb (c) {}
};
static ihash<pid_t, child, &child::pid, &child::link> chldcbs;
static time_t chldcb_check_last;

struct timecb_t {
  timespec ts;
  const cbv cb;
  itree_entry<timecb_t> link;
  timecb_t (const timespec &t, const cbv &c) : ts (t), cb (c) {}
};
static itree<timespec, timecb_t, &timecb_t::ts, &timecb_t::link> timecbs;
static bool timecbs_altered;

struct lazycb_t {
  const time_t interval;
  time_t next;
  const cbv cb;
  list_entry<lazycb_t> link;

  lazycb_t (time_t interval, cbv cb);
  ~lazycb_t ();
};
static bool lazycb_removed;
static list<lazycb_t, &lazycb_t::link> *lazylist;

const int fdsn = 2;
static cbv::ptr *fdcbs[fdsn];
static fd_set *fdsp[fdsn];
static fd_set *fdspt[fdsn];

static int sigpipes[2] = { -1, -1 };
#ifdef NSIG
const int nsig = NSIG;
#else /* !NSIG */
const int nsig = 32;
#endif /* !NSIG */
/* Note: sigdocheck and sigcaught intentionally ints rather than
 * bools.  The hope is that an int can safely be written without
 * affecting surrounding memory.  (This is certainly not the case on
 * some architectures if bool is a char.  Consider getting signal 2
 * right after signal 3 on an alpha, for instance.  You might end up
 * clearing sigcaught[2] when you finish setting sigcaught[3].) */
static volatile int sigdocheck;
static volatile int sigcaught[nsig];
static bssptr<cbv::type> sighandler[nsig];

static void sigcb_check ();

void
chldcb (pid_t pid, cbi::ptr cb)
{
  if (child *c = chldcbs[pid]) {
    chldcbs.remove (c);
    delete c;
  }
  if (cb)
    chldcbs.insert (New child (pid, cb));
}

void
chldcb_check ()
{
  for (;;) {
    int status;
    pid_t pid = waitpid (-1, &status, WNOHANG);
    if (pid == 0 || pid == -1)
      break;
    if (child *c = chldcbs[pid]) {
      chldcbs.remove (c);
#ifdef WRAP_DEBUG
      if (callback_trace & CBTR_CHLD)
	warn ("CALLBACK_TRACE: %schild pid %d (status %d) %s <- %s\n",
	      timestring (), pid, status, c->cb->dest, c->cb->line);
#endif /* WRAP_DEBUG */
      (*c->cb) (status);
      delete c;
    }
  }
  chldcb_check_last = timenow;
}

timecb_t *
timecb (const timespec &ts, cbv cb)
{
  timecb_t *to = New timecb_t (ts, cb);
  timecbs.insert (to);
  // timecbs_altered = true;
  return to;
}

timecb_t *
delaycb (time_t sec, u_int32_t nsec, cbv cb)
{
  timespec ts;
  my_clock_gettime (&ts);
  ts.tv_sec += sec;
  ts.tv_nsec += nsec;
  if (ts.tv_nsec >= 1000000000) {
    ts.tv_nsec -= 1000000000;
    ts.tv_sec++;
  }
  return timecb (ts, cb);
}

void
timecb_remove (timecb_t *to)
{
  if (!to)
    return;

  for (timecb_t *tp = timecbs[to->ts]; tp != to; tp = timecbs.next (tp))
    if (!tp || tp->ts != to->ts)
      panic ("timecb_remove: invalid timecb_t\n");
  timecbs_altered = true;
  timecbs.remove (to);
  delete to;
}

void
timecb_check ()
{
  my_clock_gettime (&tsnow);
  timecb_t *tp, *ntp;

  for (tp = timecbs.first (); tp && tp->ts <= tsnow;
       tp = timecbs_altered ? timecbs.first () : ntp) {
    ntp = timecbs.next (tp);
    timecbs.remove (tp);
    timecbs_altered = false;
#ifdef WRAP_DEBUG
    if (callback_trace & CBTR_TIME)
      warn ("CALLBACK_TRACE: %stimecb %s <- %s\n", timestring (),
	    tp->cb->dest, tp->cb->line);
#endif /* WRAP_DEBUG */
    (*tp->cb) ();
    delete tp;
  }
  selwait.tv_usec = 0;
  if (!(tp = timecbs.first ()))
    selwait.tv_sec = 86400;
  else {
    my_clock_gettime (&tsnow);
    if (tp->ts < tsnow)
      selwait.tv_sec = 0;
    else if (tp->ts.tv_nsec >= tsnow.tv_nsec) {
      selwait.tv_sec = tp->ts.tv_sec - tsnow.tv_sec;
      selwait.tv_usec = (tp->ts.tv_nsec - tsnow.tv_nsec) / 1000;
    }
    else {
      selwait.tv_sec = tp->ts.tv_sec - tsnow.tv_sec - 1;
      selwait.tv_usec = (1000000000 + tp->ts.tv_nsec - tsnow.tv_nsec) / 1000;
    }
  }
  if (sigdocheck)
    selwait.tv_sec = selwait.tv_usec = 0;
}

void
fdcb (int fd, selop op, cbv::ptr cb)
{
  assert (fd >= 0);
  assert (fd < maxfd);
  fdcbs[op][fd] = cb;
  if (cb) {
    if (fd >= nselfd)
      nselfd = fd + 1;
    FD_SET (fd, fdsp[op]);
  }
  else
    FD_CLR (fd, fdsp[op]);
}

static void
fdcb_check (void)
{
  for (int i = 0; i < fdsn; i++)
    memcpy (fdspt[i], fdsp[i], fd_set_bytes);
  int n = select (nselfd, fdspt[0], fdspt[1], NULL, &selwait);
  if (n < 0 && errno != EINTR)
    panic ("select: %m\n");
  my_clock_gettime (&tsnow);
  if (sigdocheck)
    sigcb_check ();
  for (int fd = 0; fd < maxfd && n > 0; fd++)
    for (int i = 0; i < fdsn; i++)
      if (FD_ISSET (fd, fdspt[i])) {
	n--;
	if (FD_ISSET (fd, fdsp[i])) {
#ifdef WRAP_DEBUG
	  if (fd != errfd && fd != sigpipes[0]
	      && (callback_trace & CBTR_FD))
	    warn ("CALLBACK_TRACE: %sfdcb %d%c %s <- %s\n",
		  timestring (), fd, "rwe"[i],
		  fdcbs[i][fd]->dest, fdcbs[i][fd]->line);
#endif /* WRAP_DEBUG */
	  (*fdcbs[i][fd]) ();
	}
      }
}

static void
sigcatch (int sig)
{
  sigdocheck = 1;
  sigcaught[sig] = 1;
  selwait.tv_sec = selwait.tv_usec = 0;
  /* On some operating systems, select is not a system call but is
   * implemented inside libc.  This may cause a race condition in
   * which select ends up being called with the original (non-zero)
   * value of selwait.  We avoid the problem by writing to a pipe that
   * will wake up the select. */
  write (sigpipes[1], "", 1);
}

cbv::ptr
sigcb (int sig, cbv::ptr cb, int flags)
{
  sigset_t set;
  if (!sigemptyset (&set) && !sigaddset (&set, sig))
    sigprocmask (SIG_UNBLOCK, &set, NULL);

  struct sigaction sa;
  assert (sig > 0 && sig < nsig);
  bzero (&sa, sizeof (sa));
  sa.sa_handler = cb ? sigcatch : SIG_DFL;
  sa.sa_flags = flags;
  if (sigaction (sig, &sa, NULL) < 0) // Must be bad signal, serious bug
    panic ("sigcb: sigaction: %m\n");
  cbv::ptr ocb = sighandler[sig];
  sighandler[sig] = cb;
  return ocb;
}

static void
sigcb_check ()
{
  if (sigdocheck) {
    char buf[64];
    while (read (sigpipes[0], buf, sizeof (buf)) > 0)
      ;
    sigdocheck = 0;
    for (int i = 1; i < nsig; i++)
      if (sigcaught[i]) {
	sigcaught[i] = 0;
	if (cbv::ptr cb = sighandler[i]) {
#ifdef WRAP_DEBUG
	  if (callback_trace & CBTR_SIG && i != SIGCHLD) {
# ifdef NEED_SYS_SIGNAME_DECL
	    warn ("CALLBACK_TRACE: %ssignal %d %s <- %s\n",
		  timestring (), i, cb->dest, cb->line);
# else /* !NEED_SYS_SIGNAME_DECL */
	    warn ("CALLBACK_TRACE: %sSIG%s %s <- %s\n", timestring (),
		  sys_signame[i], cb->dest, cb->line);
# endif /* !NEED_SYS_SIGNAME_DECL */
	  }
#endif /* WRAP_DEBUG */
	  (*cb) ();
	}
      }
  }
}

lazycb_t::lazycb_t (time_t i, cbv c)
  : interval (i), next (timenow + interval), cb (c)
{
  lazylist->insert_head (this);
}

lazycb_t::~lazycb_t ()
{
  lazylist->remove (this);
}

lazycb_t *
lazycb (time_t interval, cbv cb)
{
  return New lazycb_t (interval, cb);
}

void
lazycb_remove (lazycb_t *lazy)
{
  lazycb_removed = true;
  delete lazy;
}

void
lazycb_check ()
{
  my_clock_gettime (&tsnow);
 restart:
  lazycb_removed = false;
  for (lazycb_t *lazy = lazylist->first; lazy; lazy = lazylist->next (lazy)) {
    if (timenow < lazy->next)
      continue;
    lazy->next = timenow + lazy->interval;
#ifdef WRAP_DEBUG
    if (callback_trace & CBTR_LAZY)
      warn ("CALLBACK_TRACE: %slazy %s <- %s\n", timestring (),
	    lazy->cb->dest, lazy->cb->line);
#endif /* WRAP_DEBUG */
    (*lazy->cb) ();
    if (lazycb_removed)
      goto restart;
  }
}

static void
ainit ()
{
  if (sigpipes[0] == -1) {
    if (pipe (sigpipes) < 0)
      fatal ("could not create sigpipes: %m\n");

    _make_async (sigpipes[0]);
    _make_async (sigpipes[1]);
    close_on_exec (sigpipes[0]);
    close_on_exec (sigpipes[1]);
    fdcb (sigpipes[0], selread, cbv_null);

    /* Set SA_RESTART for SIGCHLD, primarily for the benefit of
     * stdio-using code like lex/flex scanners.  These tend to flip out
     * if read ever returns EINTR. */
    sigcb (SIGCHLD, wrap (chldcb_check), (SA_NOCLDSTOP
#ifdef SA_RESTART
					  | SA_RESTART
#endif /* SA_RESTART */
					  ));
    sigcatch (SIGCHLD);
  }
}

static inline void
_acheck ()
{
  if (amain_panic)
    panic ("child process returned from afork ()\n");
  lazycb_check ();
  fdcb_check ();
  sigcb_check ();

#if 0
  /*
   * On planet-lab's weird Linux kernel, sometimes processes stop
   * receiving SIGCHLD signals.  To avoid endlessly accumulating
   * Zombie children, we periodically try to reap children anyway,
   * even when we haven't received a SIGCHLD.  This is kind of gross,
   * but without the workaround one can end up completely filling the
   * process table with zombies (making it hard to log in and fix the
   * problem).
   */
  if (timenow > chldcb_check_last + 60)
    chldcb_check ();
#endif

  timecb_check ();
}

void
acheck ()
{
  timecb_check ();
  ainit ();
  _acheck ();
}

void
amain ()
{
  static bool amain_called;
  if (amain_called)
    panic ("amain called recursively\n");
  amain_called = true;

  ainit ();
  err_init ();

  timecb_check ();
  for (;;)
    _acheck ();
}

int async_init::count;

void
async_init::start ()
{
  static bool initialized;
  if (initialized)
    panic ("async_init called twice\n");
  initialized = true;

  /* Ignore SIGPIPE, since we may get a lot of these */
  struct sigaction sa;
  bzero (&sa, sizeof (sa));
  sa.sa_handler = SIG_IGN;
  sigaction (SIGPIPE, &sa, NULL);

  if (!execsafe ()) {
    int fdlim_hard = fdlim_get (1);
    if (char *p = getenv ("FDLIM_HARD")) {
      int n = atoi (p);
      if (n > 0 && n < fdlim_hard) {
	fdlim_hard = n;
	fdlim_set (fdlim_hard, -1);
      }
    }
  }
  if (!getenv ("FDLIM_HARD") || !execsafe ()) {
    str var = strbuf ("FDLIM_HARD=%d", fdlim_get (1));
    xputenv (var);
    var = strbuf ("FDLIM_SOFT=%d", fdlim_get (0));
    xputenv (var);
  }
#ifndef HAVE_WIDE_SELECT
  fdlim_set (FD_SETSIZE, execsafe ());
  maxfd = fdlim_get (0);
  fd_set_bytes = sizeof (fd_set);
#else /* HAVE_WIDE_SELECT */
  if (!execsafe () || fdlim_set (FDLIM_MAX, 1) < 0)
    fdlim_set (fdlim_get (1), 0);
  maxfd = fdlim_get (0);
  fd_set_bytes = (maxfd+7)/8;
  if (fd_set_bytes % FD_SETSIZE_ROUND)
    fd_set_bytes += FD_SETSIZE_ROUND - (fd_set_bytes % FD_SETSIZE_ROUND);
#endif /* HAVE_WIDE_SELECT */

  for (int i = 0; i < fdsn; i++) {
    fdcbs[i] = New cbv::ptr[maxfd];
    fdsp[i] = (fd_set *) xmalloc (fd_set_bytes);
    bzero (fdsp[i], fd_set_bytes);
    fdspt[i] = (fd_set *) xmalloc (fd_set_bytes);
    bzero (fdspt[i], fd_set_bytes);
  }

  lazylist = New list<lazycb_t, &lazycb_t::link>;

#ifdef WRAP_DEBUG 
  if (char *p = getenv ("CALLBACK_TRACE")) {
    if (strchr (p, 'f'))
      callback_trace |= CBTR_FD;
    if (strchr (p, 't'))
      callback_trace |= CBTR_TIME;
    if (strchr (p, 's'))
      callback_trace |= CBTR_SIG;
    if (strchr (p, 'c'))
      callback_trace |= CBTR_CHLD;
    if (strchr (p, 'l'))
      callback_trace |= CBTR_LAZY;
    if (strchr (p, 'a'))
      callback_trace |= -1;
    if (strchr (p, 'T'))
      callback_time = true;
  }
#endif /* WRAP_DEBUG */
}

void
async_init::stop ()
{
  err_flush ();
}
