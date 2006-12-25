
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

void clearread (int fd);
void clearwrite (int fd);
void waitread (int fd, cbv cb, ptr<canceller_t> *cn = NULL, CLOSURE);
void waitwrite (int fd, cbv cb, ptr<canceller_t> *cn = NULL, CLOSURE);
void proxy (int in, int out, cbv cb, ptr<canceller_t> *cncp = NULL,
	    CLOSURE);


class iofd_t {
public:
  iofd_t (int fd, selop op) : _fd (fd), _op (op), _on (false) {}
  ~iofd_t () { off (); }
  void on (cbv cb, CLOSURE);
  void off (bool check = true);
  int fd () const { return _fd; }
private:
  const int _fd;
  const selop _op;
  bool _on;
}; 

#endif /* _LIBTAME_TAME_THREAD_H_ */
