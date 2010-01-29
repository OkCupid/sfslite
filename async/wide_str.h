// -*-c++-*-
/* $Id: str.h 4555 2009-06-25 15:38:05Z max $ */

#include "str.h"
#include <wchar.h>

#ifndef __ASYNC__WIDE_STR_H__
#define __ASYNC__WIDE_STR_H__ 1

class wide_str_t {
public:
  wide_str_t (str utf8_in);
  wide_str_t (const wchar_t *in);
  wide_str_t (const wchar_t *in, size_t len);

  str to_utf8 () const;
  const wchar_t *buf (size_t *lenp = NULL) const;
  wchar_t *buf (size_t *lenp = NULL);

  size_t len () const { return _len; }
  operator bool() const { return _buf; }
  operator str() const { return to_utf8 (); }

  void chop (size_t start, size_t len = (size_t) -1);
  wide_str_t substr (size_t start, size_t len = (size_t) -1) const;
  bool error () const { return _err; }
  void setbuf (ptr<vec<wchar_t> > v, size_t sz);
  static bool init(const char *locale);

private:
  void _init (size_t len);
  void _init (const wchar_t *wc, size_t len);
  ptr<vec<wchar_t> > _buf;
  size_t _len;
  bool _err;
};

const strbuf &
strbuf_cat (const strbuf &b, const wide_str_t &s);

str utf8_substr (const str &s, size_t pos, size_t len = (size_t) -1);

// for a given input UTF-8 string, strip out any broken code points, yielding a 
// string that's guaranteed to parse.  Broken points might have been introduced
// due to naive string truncation...
str utf8_fix (const str &s, const str &repl = NULL);

#endif /* __ASYNC__WIDE_STR_H__ */
