// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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

#ifndef _ASYNC_ERR_H_
#define _ASYNC_ERR_H_ 1

#include "str.h"

extern bssstr progname;
extern str progdir;
extern void (*fatalhook) ();

extern int errfd;
extern bool fatal_no_destruct;
extern void (*_err_output) (suio *, int);
extern void (*_err_reset_hook) ();
void err_reset ();
void _err_output_sync (suio *, int);

void setprogname (char *argv0);

/* Old-style C functions for compatibility */
extern "C" {
  void sfs_warn (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
  void sfs_warnx (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
  void sfs_vwarn (const char *fmt, va_list ap);
  void sfs_vwarnx (const char *fmt, va_list ap);
  void fatal (const char *fmt, ...)
    __attribute__ ((noreturn, format (printf, 1, 2)));
  void panic (const char *fmt, ...)
    __attribute__ ((noreturn, format (printf, 1, 2)));
}

class warnobj : public strbuf {
  const int flags;
public:
  enum { xflag = 1, fatalflag = 2, panicflag = 4 };

  explicit warnobj (int);
  ~warnobj ();
  const warnobj &operator() (const char *fmt, ...) const
    __attribute__ ((format (printf, 2, 3)));
};
#define warn warnobj (0)
#define vwarn warn.vfmt
#define warnx warnobj (int (::warnobj::xflag))
#define vwarnx warnx.vfmt

#ifndef __attribute__
/* Fatalobj is just a warnobj with a noreturn destructor. */
class fatalobj : public warnobj {
public:
  explicit fatalobj (int f) : warnobj (f) {}
  ~fatalobj () __attribute__ ((noreturn));
};
#else /* __attribute__ */
# define fatalobj warnobj
#endif /* __attribute__ */
#define fatal fatalobj (int (::warnobj::fatalflag))
#define panic fatalobj (int (::warnobj::panicflag)) ("%s\n", __BACKTRACE__)

#endif /* !_ASYNC_ERR_H_ */
