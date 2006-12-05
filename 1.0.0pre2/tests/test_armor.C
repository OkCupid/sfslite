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

#include "crypt.h"

int
main ()
{
  random_update ();

  for (int i = 0; i < 1000; i++) {
    size_t len = rnd.getword () % 31;
    wmstr m (len);
    rnd.getbytes (m, len);
    str s = m;
    str a = armor64 (s);
    str b = dearmor64 (a);
    if (!b)
      panic << "dearmor64 failed:\n"
	    << "wanted: " << hexdump (s, s.len ()) << "\n"
	    << " armor: " << a << "\n";
    if (s != b)
      panic << "armor64 failure:\n"
	    << "wanted: " << hexdump (s, s.len ()) << "\n"
	    << "   got: " << hexdump (b, b.len ()) << "\n"
	    << " armor: " << a << "\n";

    a = armor64A (s);
    b = dearmor64A (a);
    if (!b)
      panic << "dearmor64A failed:\n"
	    << "wanted: " << hexdump (s, s.len ()) << "\n"
	    << " armor: " << a << "\n";
    if (s != b)
      panic << "armor64A failure:\n"
	    << "wanted: " << hexdump (s, s.len ()) << "\n"
	    << "   got: " << hexdump (b, b.len ()) << "\n"
	    << " armor: " << a << "\n";

    a = armor32 (s);
    b = dearmor32 (a);
    if (!b)
      panic << "dearmor32 failed:\n"
	    << "wanted: " << hexdump (s, s.len ()) << "\n"
	    << " armor: " << a << "\n";
    if (s != b)
      panic << "armor32 failure:\n"
	    << "wanted: " << hexdump (s, s.len ()) << "\n"
	    << "   got: " << hexdump (b, b.len ()) << "\n"
	    << " armor: " << a << "\n";
  }
}
