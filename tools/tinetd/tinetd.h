
// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "tame_io.h"
#include "ihash.h"

//=======================================================================

typedef enum { V_LO = 0, V_REG = 1, V_HI = 2} level_t;

//=======================================================================

class logger_t {
public:

  //----------------------

  class obj_t {
  public:
    obj_t (bool silent, bool x) : _silent (silent), _xflag (x), _buf (NULL) {}
    ~obj_t () { if (_buf) delete _buf; }

    //----------------------------

    template<class T> const obj_t &
    cat (const T &x)
    {
      if (!_silent) {
	if (!_buf) {
	  _buf = New warnobj (_xflag ? warnobj::xflag : 0);
	}
	(*_buf) << x;
      }
      return (*this);
    }

  private:
    const bool _silent;
    const bool _xflag;
    warnobj *_buf;
  };

  //----------------------

  logger_t () {}
  void set_level (level_t l) { _level = l; }

  void log (level_t l, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
  void logx (level_t l, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
  void log (level_t l, const str &s);
  void logx (level_t l, const str &s);
  obj_t log (level_t l);
  obj_t logx (level_t l);
  
private:
  bool silent (level_t l) { return int (l) < int (_level); }
  level_t _level;
};

//-----------------------------------------------------------------------

template<class T> const logger_t::obj_t &
operator<< (logger_t::obj_t o, const T &x)
{
  return o.cat (x);
}

extern logger_t logger;

//=======================================================================

class main_t;

//=======================================================================

class child_t {
public:
  child_t (const str &path, u_int32_t port);

  friend class main_t;
private:
  typedef enum { NONE = 0, CRASHED = 1, OK = 2 } state_t;

  const str _path;
  state_t _state;

  u_int32_t _port;
  ihash_entry<child_t> _lnk;

};

//=======================================================================

class main_t {
public:
  main_t () {}
  int config (int argc, char *argv[]);
  bool init ();
private:
  void usage ();
  bool parse_config (const str &s);
  void got_lazy_prox (vec<str> v, str loc, bool *errp);

  ihash<u_int32_t, child_t,  &child_t::_port, &child_t::_lnk> _children;
};

//=======================================================================
