// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#ifndef __ASYNC__LOGGER2_H__
#define __ASYNC__LOGGER2_H__

#include "async.h"

//=======================================================================

typedef enum { V_LO = 0, V_REG = 1, V_HI = 2} log2_level_t;

//=======================================================================

class logger2_t {
public:

  typedef log2_level_t level_t;

  //----------------------

  class obj_t {
  public:
    obj_t (bool silent, bool x) : _silent (silent), _xflag (x) {}
    ~obj_t () {}

    //----------------------------

    template<class T> const obj_t &
    cat (const T &x)
    {
      if (!_silent) {
	if (!_buf) {
	  _buf = New refcounted<warnobj> (_xflag ? warnobj::xflag : 0);
	}
	(*_buf) << x;
      }
      return (*this);
    }


    //---------------------------------------

  private:

    const bool _silent;
    const bool _xflag;
    ptr<warnobj> _buf;
  };

  //----------------------

  logger2_t (log2_level_t l = V_REG) : _level (l) {}
  void set_level (log2_level_t l) { _level = l; }

  void log (log2_level_t l, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
  void logx (log2_level_t l, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
  void log (log2_level_t l, const str &s);
  void logx (log2_level_t l, const str &s);
  obj_t log (log2_level_t l);
  obj_t logx (log2_level_t l);
  
private:
  bool silent (log2_level_t l) { return int (l) > int (_level); }
  log2_level_t _level;
};

//-----------------------------------------------------------------------

template<class T> const logger2_t::obj_t &
operator<< (logger2_t::obj_t o, const T &x)
{
  return o.cat (x);
}

//=======================================================================

#endif /*__ASYNC__LOGGER2_H__ */
