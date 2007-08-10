
#include "select.h"

namespace sfs_core {

  //-----------------------------------------------------------------------

  std_selector_t::std_selector_t ()
    : selector_t ()
  {
    init_fdsets ();
  }

  //-----------------------------------------------------------------------

  std_selector_t::std_selector_t (selector_t *old)
    : selector_t (old)
  {
    init_fdsets ();
  }

  //-----------------------------------------------------------------------

  ~std_selector_t::std_selector_t ()
  {
    xfree (_fdsp);
    xfree (_fdspt);
  }

  //-----------------------------------------------------------------------

  void
  std_selector_t::init_fdsets ()
  {
    for (int i = 0; i < fdsn; i++) {
      fdsp[i] = (fd_set *) xmalloc (fd_set_bytes);
      bzero (fdsp[i], fd_set_bytes);
      fdspt[i] = (fd_set *) xmalloc (fd_set_bytes);
      bzero (fdspt[i], fd_set_bytes);
    }
  }

  //-----------------------------------------------------------------------

  void
  std_selector_t::fdcb (int fd, selop op, cbv::ptr cb)
  {
    assert (fd >= 0);
    assert (fd < maxfd);
    _fdcbs[op][fd] = cb;
    if (cb) {
      sfs_add_new_cb ();
      if (fd >= nselfd)
	_nselfd = fd + 1;
      FD_SET (fd, _fdsp[op]);
    }
    else
      FD_CLR (fd, _fdsp[op]);
  }

  //-----------------------------------------------------------------------

  void
  std_selector_t::compact_nselfd ()
  {
    int max_tmp = 0;
    for (int i = 0; i < nselfd; i++) {
      for (int j = 0; j < fdsn; j++) {
	if (FD_ISSET(i, _fdsp[j]))
	  max_tmp = i;
      }
    }
    _nselfd = max_tmp + 1;
  }

  //-----------------------------------------------------------------------

  void
  std_selector_t::select_failure ()
  {
    warn ("select: %m\n");
    const char *typ[] = { "reading" , "writing" };
    for (int k = 0; k < 2; k++) {
      warnx << "Select Set Dump: " << typ[k] << " { " ;
      for (int j = 0; j < maxfd; j++) {
	if (FD_ISSET (j, fdspt[k])) {
	  warnx << j << " ";
	}
      }
      warnx << " }\n";
    }
    panic ("Aborting due to select() failure\n");
  }

  //-----------------------------------------------------------------------
  
  void
  std_selector_t::fdcb_check (struct timeval *selwait)
  {

    //
    // If there was a request to compact nselfd every compact_interval,
    // then examine the fd sets and make the adjustment.
    //
    if (_compact_interval && (++_n_fdcb_iter % _compact_interval) == 0) 
      compact_nselfd ();
  
    for (int i = 0; i < fdsn; i++)
      memcpy (_fdspt[i], _fdsp[i], fd_set_bytes);

    if (_busywait) {
      memset (selwait, 0, sizeof (*selwait));
    }
    
    int n = SFS_SELECT (_nselfd, _fdspt[0], _fdspt[1], NULL, selwait);

    // warn << "select exit rc=" << n << "\n";
    if (n < 0 && errno != EINTR) {
      select_failure ();
    }
    sfs_set_global_timestamp ();
    
    try_sigcb_check ();

    for (int fd = 0; fd < maxfd && n > 0; fd++)
      for (int i = 0; i < fdsn; i++)
	if (FD_ISSET (fd, _fdspt[i])) {
	  n--;
	  if (FD_ISSET (fd, _fdsp[i])) {
#ifdef WRAP_DEBUG
	    callback_trace_fdcb (i, fd, _fdcbs[i][fd]);
#endif /* WRAP_DEBUG */
	    STOP_ACHECK_TIMER ();
	    sfs_leave_sel_loop ();
	    (*_fdcbs[i][fd]) ();
	    START_ACHECK_TIMER ();
	  }
	}
  }
  
  //-----------------------------------------------------------------------
  
};
