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

#ifndef _ASYNC_PARSEOPT_
#define _ASYNC_PARSEOPT_ 1

#include "vec.h"
#include "str.h"
#include "ihash.h"
#include "amisc.h"

class parseargs {
  static char *errorbuf;

  char *buf;
  const char *lim;
  const char *p;

  void skipblanks ();
  void skiplwsp ();
  str getarg ();

protected:
  str filename;
  int lineno;

  virtual void error (str);

public:
  parseargs (str file, int fd = -1);
  virtual ~parseargs ();

  bool getline (vec<str> *args, int *linep = NULL);
};

int64_t strtoi64 (const char *nptr, char **endptr = NULL, int base = 0);

template<class T> bool
convertint (const char *cp, T *resp)
{
  if (!*cp)
    return false;
  char *end;
  T res = strtoi64 (cp, &end, 0);
  if (*end)
    return false;
  *resp = res;
  return true;
}

void mytolower (char *dest, const char *src);
str mytolower (const str &in);

class conftab_el {
public:
  conftab_el (const str &n) : name (n), lcname (mytolower (n)) {}
  virtual ~conftab_el () {}
  virtual bool convert (const vec<str> &v, const str &loc, bool *e) = 0;
  virtual bool inbounds () = 0;
  virtual void set () = 0;
  inline bool count_args (const vec<str> &v, u_int l)
  { return (v.size () == l || (v.size () > l && v[l][0] == '#')); }

  const str name;
  const str lcname;
  ihash_entry<conftab_el> lnk;
};

typedef callback<void, vec<str>, str, bool *>::ref confcb;
class conftab_str : public conftab_el
{
public:
  conftab_str (const str &n, str *d, bool c = false) 
    : conftab_el (n), dest (d), cnfcb (NULL), scb (NULL), check (c) {}
  conftab_str (const str &n, confcb c)
    : conftab_el (n), dest (NULL), cnfcb (c), scb (NULL) {}
  conftab_str (cbs c, const str &n) // XXX: reverse order to disambiguate
    : conftab_el (n), dest (NULL), cnfcb (NULL), scb (c) {}

  bool convert (const vec<str> &v, const str &l, bool *e);
  bool inbounds () { return true; }
  void set ();

private:
  str *const dest;
  confcb::ptr cnfcb;
  cbs::ptr scb;
  const bool check;

  vec<str> tmp;
  str tmp_s;
  str loc;
  bool *errp;
};

template<class T>
class conftab_int : public conftab_el
{
public:
  conftab_int (const str &n, T *d, T l, T u)
    : conftab_el (n), dest (d), lb (l), ub (u) {}

  bool convert (const vec<str> &v, const str &cf, bool *e)
  { return (count_args (v, 2) && convertint (v[1], &tmp)); }
  bool inbounds () { return (tmp >= lb && tmp <= ub); }
  void set () { *dest = tmp; }

private:
  T *const dest;
  const T lb;
  const T ub;
  T tmp;
};

class conftab_bool : public conftab_el
{
public:
  conftab_bool (const str &n, bool *b) 
    : conftab_el (n), dest (b), err (false) {}
  bool convert (const vec<str> &v, const str &cf, bool *e);
  bool inbounds () { return !(err); }
  void set () { *dest = tmp; }
private:
  bool tmp;
  bool *dest;
  bool err;
};

class conftab
{
public:
  conftab () {}

  template<class P, class D> 
  conftab & add (const str &nm, P *dp, D lb, D ub)
  { tab.insert (New conftab_int<P> (nm, dp, lb, ub)); return *this; }
  template<class A>
  conftab & add (const str &nm, A a) 
  { tab.insert (New conftab_str (nm, a)); return *this; }
  conftab & ads (const str &nm, cbs c) // XXX: cannot overload
  { tab.insert (New conftab_str (c, nm)); return *this; }
  conftab & add (const str &nm, str *s, bool b)
  { tab.insert (New conftab_str (nm, s, b));  return *this; }
  conftab & add (const str &nm, bool *b) 
  { tab.insert (New conftab_bool (nm, b)); return *this; }

  ~conftab () { tab.deleteall (); }

  bool match (const vec<str> &s, const str &cf, int ln, bool *err);
private:
  ihash<const str, conftab_el, &conftab_el::lcname, &conftab_el::lnk> tab;
};


#endif /* !_ASYNC_PARSEOPT_ */
