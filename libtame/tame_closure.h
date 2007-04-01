
// -*-c++-*-
/* $Id: tame_core.h 2654 2007-03-31 05:42:21Z max $ */

/*
 *
 * Copyright (C) 2005 Max Krohn (max@okws.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _LIBTAME_CLOSURE_H_
#define _LIBTAME_CLOSURE_H_

#include "tame_event.h"


// All closures are numbered serially so that our accounting does not
// get confused.
extern u_int64_t closure_serial_number;


class closure_t : public virtual refcount , 
		  public weakrefcount
{
public:
  closure_t (const char *filename, const char *fun) ;

  // because it is weak_refcounted, a bool will turn to true once
  // this class is deleted.
  ~closure_t () {}

  // manage function reentry
  void set_jumpto (int i) { _jumpto = i; }
  u_int jumpto () const { return _jumpto; }

  u_int64_t id () { return _id; }

  // given a line number of the end of scope, perform sanity
  // checks on scoping, etc.
  void end_of_scope_checks (int line)
  {
    report_leaks (&_events);
  }

  // Initialize a block environment with the ID of this block
  // within the given function.  Also reset any internal counters.
  void init_block (int blockid, int lineno);

  u_int _jumpto;

  // Display full location with filename, line number and also
  // function name.
  str loc (int lineno) const;
  void error (int lineno, const char *msg);

  // Decremenet the block count; return TRUE if it goes down to 0, signifying
  // contuination inside the function.
  bool block_dec_count (const char *loc);

protected:

  u_int64_t _id;
  
  const char *_filename;              // filename for the function
  const char *_funcname;         

  // Variables involved with managing BLOCK blocks. Note that only one
  // can be active at any given time.
public:
  struct block_t { 
    block_t () : _id (0), _count (0), _lineno (0) {}
    int _id, _count, _lineno;
  };
  block_t _block;

  list<_event_cancel_base, &_event_cancel_base::_lnk> _events;

};

template<class C>
class closure_action {
public:
  closure_action (ptr<C> c) : _closure (c) {}

  ~closure_action () {}

  void perform (_event_cancel_base *event, const char *loc, bool _reuse)
  {
    if (!_closure) {
      tame_error (loc, "event reused after deallocation");
    } else {
      maybe_reenter (loc);
      clear (e);
    }
  }

  void clear (_event_cancel_base *e) 
  {
    if (_closure) {
      _closure.remove (e);
      _closure = NULL;
    }
  }

private:

  void maybe_reenter (const char *loc)
  {
    if (_closure->block_dec_count (loc))
      _closure->reenter ();
  }

  ptr<C> _closure;
};

extern ptr<closure_t> __cls_g;
extern ptr<closure_t null_closure;


#endif /* _LIBTAME_CLOSURE_H_ */
