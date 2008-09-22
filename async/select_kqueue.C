
#include "sfs_select.h"
#include <time.h>
#include "litetime.h"
#include "async.h"


#ifdef HAVE_KQUEUE

static void
kq_warn (const struct kevent &kev)
{
  warn << "kq error: "
       << "fd=" << kev.ident << "; "
       << "filter=" << kev.filter << "; "
       << "flags=" << kev.flags << "; "
       << "data=" << kev.data << "\n";
}


namespace sfs_core {

  kqueue_selector_t::kqueue_selector_t (selector_t *old)
    : selector_t (old),
      _maxevents (maxfd *2),
      _change_indx (0)
  {
    if ((_kq = kqueue ()) < 0)
      panic ("kqueue: %m\n");
    size_t sz = _maxevents * sizeof (struct kevent);
    _kq_events_out = (struct kevent *)malloc (sz);
    memset (_kq_events_out, 0, sz);
    memset (_kq_changes, 0, CHANGE_Q_SZ * sizeof (struct kevent));
  }

  kqueue_selector_t::~kqueue_selector_t ()
  {
    xfree (_kq_events_out);
  }

  void
  kqueue_selector_t::fdcb (int fd, selop op, cbv::ptr cb)
  {
    assert (fd >= 0);
    assert (fd < maxfd);

    short filter = (op == selread) ? EVFILT_READ : EVFILT_WRITE;
    u_short flags = cb ? EV_ADD : EV_DELETE;

    assert (_change_indx < CHANGE_Q_SZ);

    EV_SET(&_kq_changes[_change_indx++], fd, filter, flags, 0, 0, 0);

    _fdcbs[op][fd] = cb;
    
    if (_change_indx >= CHANGE_Q_SZ) {
      int rc;
      do {
	rc = kevent (_kq, _kq_changes, _change_indx, NULL, 0, NULL);
	if (rc < 0) {
	  if (errno == EINTR) { 
	    warn ("kqueue resumable error %d (%m)\n", errno);
	  } else {
	    panic ("kqueue failure: %m\n");
	  }
	} 
      } while (rc < 0);
      _change_indx = 0;
    }
  }

  static void
  val2spec (const struct timeval *in, struct timespec *out)
  {
    out->tv_sec = in->tv_sec;
    out->tv_nsec = 1000 * in->tv_usec;
  }

  void
  kqueue_selector_t::fdcb_check (struct timeval *selwait)
  {
    struct timespec  ts;
    val2spec (selwait, &ts);
    int rc = kevent (_kq, _kq_changes, _change_indx, _kq_events_out, 
		     _maxevents, &ts);
    if (rc < 0) {
      if (errno == EINTR) { 
	warn ("kqueue resumable error %d (%m)\n", errno);
      } else if (errno != EINTR) {
	panic ("kqueue failure: %m\n");
      }
    } else {
      _change_indx = 0;
    }

    sfs_set_global_timestamp ();
    sigcb_check ();

    for (int i = 0; i < rc; i++) {
      const struct kevent &kev = _kq_events_out[i];
      int op = -1;
      if (kev.flags & EV_ERROR) {
	kq_warn (kev);
      } else {
	switch (kev.filter) {
	case EVFILT_READ:
	  op = int (selread);
	  break;
	case EVFILT_WRITE:
	  op = int (selwrite);
	  break;
	default:
	  break;
	}
      }
      if (op >= 0 && _fdcbs[op][kev.ident]) {
	sfs_leave_sel_loop ();
	(*_fdcbs[op][kev.ident])();
      }
    }
  }
  

};



#endif /* HAVE_KQUEUE */
