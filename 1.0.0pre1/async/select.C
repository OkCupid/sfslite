
#include "select.h"

#ifdef HAVE_PTH

int sfs_cb_ins;

int sfs_core_select; // XXX bkwds compat

static int
check_cb_ins (void *dummy)
{
  return sfs_cb_ins;
}

int 
sfs_pth_core_select (int nfds, fd_set *rfds, fd_set *wfds,
		     fd_set *efds, struct timeval *timeout)
{
  // Set an arbitrary timeout of 10 seconds
  int pth_timeout = 10;

  // clear the CB insertion flag; it's set to 1 when a new CB is
  // inserted via timecb, sigcb or fdcb
  sfs_cb_ins = 0;

  pth_event_t ev = pth_event (PTH_EVENT_FUNC, check_cb_ins, 0, 
			      pth_time (pth_timeout, 0));
  
  return pth_select_ev(nfds, rfds, wfds, efds, timeout, ev);
		       
}

void sfs_add_new_cb () { sfs_cb_ins = 1; }

#else /* HAVE_PTH */

void sfs_add_new_cb () {}

#endif /* HAVE_PTH */
