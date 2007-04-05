
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

#ifndef _LIBTAME_RUN_H_
#define _LIBTAME_RUN_H_

#include "async.h"
#include "qhash.h"

/*
 * tame runtime flags
 */
#define   TAME_ERROR_SILENT      (1 << 0)
#define   TAME_ERROR_FATAL       (1 << 1)
#define   TAME_CHECK_LEAKS       (1 << 2)
#define   TAME_OPTIMIZE          (1 << 3)
#define   TAME_STRICT            (1 << 4)

extern bool tame_collect_rv_flag;
extern int tame_options;
inline bool tame_check_leaks () { return tame_options & TAME_CHECK_LEAKS ; }
inline bool tame_optimized () { return tame_options & TAME_OPTIMIZE; }
inline bool tame_strict_mode () { return tame_options & TAME_STRICT; }
void tame_error (const char *loc, const char *msg);

/**
 * functions defined in tame.C, mainly for reporting errors, and
 * determinig what will happen when an error occurs. Change the
 * runtime behavior of what happens in an error via TAME_OPTIONS
 */
INIT(tame_init);
#define TAME_OPTIONS         "TAME_OPTIONS"

class tame_stats_t {
public:
  tame_stats_t ();

  void dump ();
  void enable () { _collect = true; }
  void disable () { _collect = false; }
  inline void evv_rec_hit () { if (_collect) _evv_rec_hit (); }
  inline void evv_rec_miss() { if (_collect) _evv_rec_miss(); }
  inline void mkevent_impl_rv_alloc(const char *loc)
  { if (_collect) _mkevent_impl_rv_alloc (loc); }

private:

  void _evv_rec_hit () { _n_evv_rec_hit ++; }
  void _evv_rec_miss () { _n_evv_rec_miss ++; }
  void _mkevent_impl_rv_alloc (const char *loc);

  bool _collect;
  int _n_evv_rec_hit, _n_evv_rec_miss;
  qhash<const char *, int> _mkevent_impl_rv;
};

extern tame_stats_t *g_stats;


#endif /* _LIBTAME_RUN_H_ */
