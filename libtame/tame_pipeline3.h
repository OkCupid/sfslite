// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#pragma once

#include "async.h"
#include "tame.h"
#include "tame_connectors.h"
#include "refcnt.h"

//=======================================================================
//
// A class to automate pipelined operations.
//
namespace pipeline3 {

  //-----------------------------------------------------------------------

  class control_t : public virtual refcount {
  public:
    virtual size_t get_window_size () const = 0;
    virtual time_t get_delay_usec () const = 0;
    virtual void landed (size_t i, size_t nout, time_t usec) {}
  };

  //-----------------------------------------------------------------------

  class runner_t {
  public:
    runner_t (ptr<control_t> c) : _control (c), _n_out (0), _i (0) {}
    void queue_for_takeoff (evv_t ev, CLOSURE);

    evv_t mkev ();

    template<class T1> 
    typename event<T1>::ref 
    mkev (T1 &t1)
    { return connector::extend (mkev(), t1); }

    template<class T1, class T2> 
    typename event<T1,T2>::ref 
    mkev (T1 &t1, T2 &t2)
    { return connector::extend (mkev(), t1, t2); }

    template<class T1, class T2, class T3> 
    typename event<T1,T2,T3>::ref 
    mkev (T1 &t1, T2 &t2, T3 &t3)
    { return connector::extend (mkev(), t1, t2, t3); }

    void flush (evv_t ev, CLOSURE);
  private:
    void _mkev (evv_t::ptr *ev, CLOSURE);

    ptr<control_t> _control;
    size_t _n_out;
    size_t _i;
    evv_t::ptr _ev;
  };

  //-----------------------------------------------------------------------

  class passive_control_t : public control_t {
  public:
    passive_control_t (size_t ws = 5, time_t delay = 0)
      : _window_size (ws), _delay_usec (delay) {}
    size_t get_window_size () const { return _window_size; }
    time_t get_delay_usec () const { return _delay_usec; }
    static ptr<passive_control_t> alloc (size_t w, time_t d)
    { return New refcounted<passive_control_t> (w, d); }
  private:
    size_t _window_size;
    time_t _delay_usec;
  };

}

//=======================================================================
