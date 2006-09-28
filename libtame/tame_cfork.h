
// -*-c++-*-
/* $Id: tame_core.h 2225 2006-09-28 15:41:28Z max $ */

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

#ifndef _LIBTAME_TAME_CFORK_H_
#define _LIBTAME_TAME_CFORK_H_

#include "tame_thread.h"
#include "tame_core.h"
#include "tame_mkevent.h"
#include "async.h"

template<class R>
class cthread_t {
public:
  cthread_t (event_void_t e, R &r, typename callback<R, void>::ref a) 
    : _event (e), _result (r), _action (a) {}

  static void * run (void *me) 
  { (reinterpret_cast<cthread_t<R> *> (me))->_run (); return NULL; }

  void _run ()
  {
    _result = (*_action) ();
    SIGNAL (_event);
    delete this;
    tame_thread_exit ();
  }
  
private:
  event_void_t _event;
  R &_result;
  typename callback<R, void>::ref _action;
};

template<class R>
void __cfork (const char *loc, event_void_t e, R &r, cbv a)
{
  cthread_t<R> *t = New cthread_t<R> (e, r, a);
  tame_thread_spawn (loc, cthread_t<R>::run, static_cast<void *> (t)); 
}

template<>
class cthread_t<void> {
public:
  cthread_t (event_void_t e, cbv a) : _event (e), _action (a) {}
  
  static void * run (void *me)
  { (reinterpret_cast<cthread_t<void> *> (me))->_run (); return NULL; }

  void _run ()
  {
    (*_action) ();
    SIGNAL (_event);
    delete this;
    tame_thread_exit ();
  }
private:
  event_void_t _event;
  cbv _action;
};

void __cfork (const char *loc, event_void_t e, cbv a);

void _cfork (implicit_rendezvous_t *i, const char *loc, cbv a);
void _cfork (ptr<closure_t> c, const char *loc, rendezvous_t<> rv, cbv a);

template<class R>
void _cfork (implicit_rendezvous_t *i, const char *loc, R &r, cbv a)
{
  __cfork (loc, _mkevent (i, loc), r, a);
}

#include "tame_cfork_ag.h"

#define cfork(...) _cfork (__cls_g, HERE, ##__VA_ARGS__)



#endif /* _LIBTAME_TAME_CFORK_H_ */
