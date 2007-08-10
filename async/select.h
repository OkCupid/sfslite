// -*-c++-*-
/* $Id: async.h 2247 2006-09-29 20:52:16Z max $ */

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

#ifndef _ASYNC_SELECT_H_
#define _ASYNC_SELECT_H_ 1

#include "config.h"
#include "amisc.h"

//-----------------------------------------------------------------------
//
//  Deal with select(2) call, especially if in the case of PTH threads
//  and wanted to use PTH's select loop instead of the standard libc 
//  variety.
//

void sfs_add_new_cb ();

#ifdef HAVE_TAME_PTH

# include <pth.h>
# define SFS_SELECT sfs_pth_core_select

int sfs_pth_core_select (int nfds, fd_set *rfds, fd_set *wfds,
			 fd_set *efds, struct timeval *timeout);

#else /* HAVE_TAME_PTH */

# define SFS_SELECT select

#endif /* HAVE_TAME_PTH */

//
// end select(2) stuff
//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//
//  Now, figure out the specifics of the select-on-FD operations, and
//  whether that will be using select(2) or is using something fancier
//  like epoll or kqueue.
//

#ifdef HAVE_KQUEUE
# include <sys/types.h>
# include <sys/event.h>
# include <sys/time.h>
#endif

namespace sfs_core {

  //-----------------------------------------------------------------------
  // 
  // API available to users:
  //
  typedef enum { SELECT_NONE, 
		 SELECT_STD, 
		 SELECT_EPOLL, 
		 SELECT_KQUEUE } select_policy_t;

  void set_busywait (bool b);   
  void set_compact_interval (u_int i);
  int  set_select_policy (select_policy_t i);

  //
  // end public API
  //-----------------------------------------------------------------------
  
  class selector_t {
  public:
    selector_t ();
    selector_t (const selector_t *s);
    virtual ~selector_t ();
    virtual void fdcb (int, selop, cbv::ptr) = 0;
    virtual void fdcb_check (struct timeval *timeout) = 0;
    virtual int set_compact_interval (u_int i) { return -1; }
    virtual int set_busywait (bool b) { return -1; }
    virtual select_policy_t typ () const = 0;

    static int fd_set_bytes;
    static int maxfd;
    static void init (void);

    cbv::ptr *fdcbs () { return _fdcbs; }

    enum { fdsn = 2 ; }
  protected:
    cbv::ptr *_fdcbs[fdsn];
  };

  class std_selector_t : public selector_t {
  public:
    std_selector_t ();
    std_selector_t (const select_t *s);
    void fdcb (int selop, cbv::ptr);
    void fdcb_check (struct timeval *timeout);
    int set_compact_interval (u_int i);
    int set_busywait (bool b);
    select_policy_t typ () const { return SELECT_STD; }

  private:
    
    void compact_nselfd ();

    u_int _compact_interval;
    u_int _n_fdcb_iter;
    int _nselfd;
    bool _busywait;

    int _nselfd;
    fd_set *_fdsp[fdsn];
    fd_set *_fdspt[fdsn];
  };

#ifdef HAVE_EPOLL
# include <sys/epoll.h>

  class epoll_selector_t : public selector_t {
  public:
    epoll_selector_t (selector_t *cur);
    ~epoll_selector_t ();
    void fdcb (int selop, cbv::ptr);
    void fdcb_check (struct timeval *timeout);
    select_policy_t typ () const { return SELECT_EPOLL; }

  private:

    int _epfd;
    struct epoll_event *_ret_events;
    int _maxevents;
    struct epoll_state {
      int  user_events; /* holds bits for READ and WRITE */
      bool in_epoll;
    };
    epoll_state *_epoll_states;
  };
#endif /* HAVE_EPOLL */

};

//
//-----------------------------------------------------------------------


#endif /* _ASYNC_SELECT_H_ */
