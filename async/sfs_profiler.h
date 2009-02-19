// -*-c++-*-
/* $Id: async.h 4052 2009-02-12 13:22:01Z max $ */

#include "async.h"

#ifndef __ASYNC__SFS_PROFILER_H__
#define __ASYNC__SFS_PROFILER_H__

#ifdef SIMPLE_PROFILER

//
// This is the exposed interface to the sfs profiler
//
class sfs_profiler {
public:
  static void enable ();
  static void disable ();
  static void report ();
  static void reset ();
  static void set_interval (time_t us);
  static void set_real_timer (bool b);
};

#endif /* SIMPLE_PROFILER */

#endif /* __ASYNC__SFS_PROFILER_H__ */
