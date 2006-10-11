
// -*-c++-*-
/* $Id: async.h 1759 2006-05-19 23:54:14Z max $ */

// NEED SFS AUTOCONF.H!
#include "autoconf.h"

#ifndef _CALLBACK_H_INCLUDED_

# ifdef SFS_HAVE_CALLBACK2
#  include "callback2.h"
#  define TRIGGER(cb, ...) (cb)->trigger (__VA_ARGS__)
# else
#  include "callback1.h"
#  define TRIGGER(cb, ...) (*cb) (__VA_ARGS__)
# endif /* SFS_HAVE_CALLBACK2 */

#endif /* _CALLBACK_H_INCLUDED_ */
