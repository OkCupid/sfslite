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

void sfs_add_new_cb ();

#ifdef HAVE_TAME_PTH

# include <pth.h>
# define SFS_SELECT sfs_pth_core_select

int sfs_pth_core_select (int nfds, fd_set *rfds, fd_set *wfds,
			 fd_set *efds, struct timeval *timeout);

#else /* HAVE_TAME_PTH */

# define SFS_SELECT select

#endif /* HAVE_TAME_PTH */


#endif /* _ASYNC_SELECT_H_ */
