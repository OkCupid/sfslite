
// -*-c++-*-
/* $Id: async.h 1759 2006-05-19 23:54:14Z max $ */

#ifdef SFS_HAVE_CALLBACK2
# include "callback2.h"
# define SIGNAL(cb, ...) (cb)->signal (__VA_ARGS__)
#else
# include "callback1.h"
# define SIGNAL(cb, ...) (*cb) (__VA_ARGS__)
#endif /* SFS_HAVE_CALLBACK2 */
