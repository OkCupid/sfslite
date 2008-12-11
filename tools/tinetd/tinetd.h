// -*-c++-*-
/* $Id: async.h 3492 2008-08-05 21:38:00Z max $ */

#include "async.h"
#include "tame.h"
#include "parseopt.h"
#include "tame_io.h"
#include "ihash.h"
#include "list.h"
#include <arpa/inet.h>

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
class child_t;

//=======================================================================

typedef u_int32_t port_t;

//=======================================================================

class cli_t {
public:
  cli_t (child_t *ch, int cfd, const str &a, int sfd);
  ~cli_t ();

  void run (evv_t ev, CLOSURE);

  friend class child_t;

private:
  child_t *_server;
  int _cli_fd;
  str _cli_addr;
  int _srv_fd;
  
  list_entry<cli_t> _lnk;
};

//=======================================================================

class child_t {
public:
  child_t (main_t *m, port_t port, const vec<str> &v);
  port_t port () const { return _port; }
  bool init ();
  bool run ();
  void newcon () { newcon_T (); }
  void insert (cli_t *cli) { _clients.insert_head (cli); }
  void remove (cli_t *cli) { _clients.remove (cli); }
  str srvname () const { return _cmd[0]; }

  friend class main_t;
private:
  void newcon_T (CLOSURE);
  void get_srv_fd (evi_t ev);
  void launch_loop (CLOSURE);
  void launch (evv_t ev, CLOSURE);
  void wait_for_crash (evv_t ev, CLOSURE);


  typedef enum { NONE = 0, CRASHED = 1, OK = 2 } state_t;

  main_t *_main;
  port_t _port;
  ihash_entry<child_t> _lnk;

  vec<str> _cmd;
  state_t _state;
  int _lfd;
  vec<evi_t::ptr> _waiters;
  int _srv_fd;
  pid_t _pid;
  evv_t::ptr _poke_ev;

  list<cli_t, &cli_t::_lnk> _clients;
};

//=======================================================================

class main_t {
public:
  main_t () {}
  int config (int argc, char *argv[]);
  bool init ();
  bool run ();
  int crash_wait () const { return _crash_wait; }
  const struct in_addr &addr () const { return _addr; }
private:
  bool insert (child_t *ch);
  void usage ();
  bool parse_config (const str &s);
  void got_lazy_prox (vec<str> v, str loc, bool *errp);
  bool ch_apply (bool (child_t::*fn) () );

  typedef ihash<port_t, child_t, &child_t::_port, &child_t::_lnk> hsh_t;
  typedef ihash_iterator_t<child_t, hsh_t> hiter_t;
  
  hsh_t _children;
  struct in_addr _addr;
  bool _daemonize;
  int _crash_wait;
};

//=======================================================================
