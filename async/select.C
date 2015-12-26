
#include "sfs_select.h"
#include "fdlim.h"

//-----------------------------------------------------------------------
//
// Different select(2) options for core loop
//

#define FD_SETSIZE_ROUND (sizeof (long))

namespace sfs_core {

  void
  selector_t::init (void)
  {
    maxfd = fdlim_get (0);

#if defined(HAVE_WIDE_SELECT) || defined(HAVE_EPOLL) || defined(HAVE_KQUEUE)
    if (!execsafe () || fdlim_set (FDLIM_MAX, 1) < 0)
      fdlim_set (fdlim_get (1), 0);
    fd_set_bytes = (maxfd+7)/8;
    if (fd_set_bytes % FD_SETSIZE_ROUND)
      fd_set_bytes += FD_SETSIZE_ROUND - (fd_set_bytes % FD_SETSIZE_ROUND);

# else /* !WIDE_SELECT and friends */
    fdlim_set (FD_SETSIZE, execsafe ());
    fd_set_bytes = sizeof (fd_set);

#endif /* !WIDE_SELECT */

  }

  //-----------------------------------------------------------------------

  selector_t::selector_t () : _maxfd_at_construction (maxfd)
  {
    for (int i = 0; i < fdsn; i++) {
      _fdcbs[i] = New cbv::ptr[maxfd];
    }
  }

  selector_t::selector_t (selector_t *old)
  {
    for (int i = 0; i < fdsn; i++) {
      _fdcbs[i] = old->fdcbs () [i];
    }
  }

  selector_t::~selector_t () {}

  //-----------------------------------------------------------------------

  int selector_t::fd_set_bytes;
  int selector_t::maxfd;

  //-----------------------------------------------------------------------
  
};

//
//
//-----------------------------------------------------------------------
