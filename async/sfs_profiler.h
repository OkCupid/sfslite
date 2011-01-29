// -*-c++-*-
/* $Id: async.h 4052 2009-02-12 13:22:01Z max $ */

#include "async.h"

#ifndef __ASYNC__SFS_PROFILER_H__
#define __ASYNC__SFS_PROFILER_H__

//
// This is the exposed interface to the sfs profiler
//
class sfs_profiler {
public:

  //
  // The core of the profiler --- what to do when we actually need to collect
  // the data.  Also, a hook in the main look to potentially recharge
  // the memory of the profiler.  Might eventually add more hooks here
  // so we can virtualize in OKWS and other applications.
  //
  class core_t {
  public:
    virtual void profile_hook (const void *in) = 0;
    virtual void recharge () = 0;
    virtual void report () = 0;
    virtual void reset () = 0;
  };

  static bool enable ();
  static void disable ();
  static void report ();
  static void reset ();
  static void recharge ();
  static void set_interval (time_t us);
  static void set_real_timer (bool b);

  // keep track of when we enter/exit libs with -fomit-frame-pointer
  // call these libs: vomit libs
  static void enter_vomit_lib ();
  static void exit_vomit_lib ();
  static void init ();
  static void set_core (sfs_profiler::core_t *c);

};

//
// These functions are wrapped with vomit entry/exits so they can
// be properly profiled.
//
namespace sfs {
  void *memcpy_p (void *dest, const void *src, size_t n);
};


#endif /* __ASYNC__SFS_PROFILER_H__ */
