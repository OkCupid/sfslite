// -*-c++-*-
/* $Id: tame_valueset.h 2673 2007-04-03 04:59:39Z max $ */

#pragma once
#include "tame_closure.h"
#include "ihash.h"

namespace tame {

  //-----------------------------------------------------------------------

  struct ustime_t {
    ustime_t ();
    ustime_t (const struct timespec &ts) { set (ts); }
    void set (const struct timespec &ts);
    u_int64_t _time;
  };

  //-----------------------------------------------------------------------

  class profile_site_t {
  public:
    profile_site_t (str l, const char *f) : _loc (l), _func (f) {}
    const str _loc;
    const char *_func;

    // these fields are set by mk_report whenever it runs...
    size_t _n;
    u_int64_t _msec_tot;
    u_int64_t _msec_avg;

    void add_instance (const closure_t *c);
    void remove_instance (const closure_t *c);
    void prep_report ();
    void report ();
    qhash<u_int64_t, ustime_t> _instances;
    ihash_entry<profile_site_t> _lnk;
  };

  //-----------------------------------------------------------------------

  class profiler_t {
  public:
    profiler_t ();
    static profiler_t *profiler (); // global object
    void enter_closure (const closure_t *c);
    void exit_closure (const closure_t *c);
    void report ();
    void enable ();
    void disable ();
    void clear ();
    
    typedef ihash<const str, profile_site_t, &profile_site_t::_loc, 
		  &profile_site_t::_lnk> tab_t;
    tab_t _tab;
    bool _enabled;
  };

  //-----------------------------------------------------------------------

};
