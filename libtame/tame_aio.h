// -*-c++-*-
/* $Id: tame_io.h 2225 2006-09-28 15:41:28Z max $ */
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

#ifndef _LIBTAME_TAME_IO_H_
#define _LIBTAME_TAME_IO_H_

#include "tame.h"
#include "aiod.h"

namespace tame {

  typedef event_t<ptr<aiobuf>, ssize_t>::ref aio_read_ev_t;

  class aiofh_t {
  public:
    aiofh_t (aiod *a) : _aiod (a), _off (0) {}
    void open (const str &fn, int flg, int mode, evi_t ev, CLOSURE);
    void read (size_t sz, aio_read_ev_t ev, CLOSURE);
    void lseek (off_t o) { _off = o; }

  private:
    aiod *_aiod;
    ptr<aiofh> _fh;
    ptr<aiobuf> _buf;
    size_t _bufsz;
    off_t _off;
    str _fn;
  };



};



#endif /* _LIBTAME_TAME_THREAD_H_ */