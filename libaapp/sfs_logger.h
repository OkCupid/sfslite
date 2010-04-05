// -*-c++-*-
#pragma once

#include "async.h"
#include "arpc.h"
#include "tame.h"
#include "tame_lock.h"
#include "aapp_prot.h"

namespace sfs {

  class logger_t {
  public:
    logger_t (str n, int m = 0644, int tries = 5);
    ~logger_t ();
    void launch (evb_t ev, bool do_lock = true, CLOSURE);
    void turn (evb_t ev, CLOSURE);
    void log (str s, evb_t ev, CLOSURE);
    void eofcb (ptr<bool> df);
  private:
    const str _file;
    const int _mode;
    tame::lock_t _lock;
    int _tries;
    pid_t _pid;
    ptr<bool> _destroyed;
    ptr<axprt_unix> _x;
    ptr<aclnt> _cli;
  };

};
