

// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#ifndef _LIBTAME_PC_H_
#define _LIBTAME_PC_H_

#include "async.h"
#include "list.h"
#include "tame.h"

namespace tame {

  // producer/consumer with 1 object
  template<class V>
  class pc1_t {
  public:
    pc1_t (const V &v) : _set (false), _done (false), _val (v) {}

    void set (const V &v) 
    {
      assert (!_set);
      assert (!_done);

      if (_cb) {
	_done = true;
	typename callback<void, bool, V>::ref c (_cb);
	_cb = NULL;
	c->trigger (true, _val);
      } else {
	_set = true;
	_val = v;
      }
    }

    void get (typename callback<void, bool, V>::ref c)
    {
      bool ret (false);
      if (!_done) {
	if (_set) {
	  assert (!_cb);
	  _done = true;
	  ret = true;
	} else if (!_cb) {
	  _cb = c;
	  return;
	}
      }
      c->trigger (ret, _val);
    }
   
  private:
    bool _set;
    bool _done;
    V _val;
    typename callback<void, bool, V>::ptr _cb;
  };  

};


#endif /* _LIBTAME_PC_H_ */

