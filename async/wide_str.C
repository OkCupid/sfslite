#include "wide_str.h"
#include <locale.h>
#include "err.h"

void
wide_str_t::init ()
{
  static bool is_init = false;
  if (!is_init) {
    is_init = true;
    setlocale (LC_CTYPE, "UTF-8");
  }
}

void
wide_str_t::init (size_t l)
{
  init ();
  _buf = New refcounted<vec<wchar_t> > ();
  _buf->setsize (l+1);
  memset (_buf->base (), 0, sizeof (wchar_t) * (l+1));
  _len = l;
  _err = false;
}

wide_str_t::wide_str_t (str utf8_in)
{
  if (utf8_in) {
    init (utf8_in.len ());
    mbstate_t state;
    memset (&state, 0, sizeof (state));
    const char *src = utf8_in.cstr ();
    setlocale (LC_CTYPE, "UTF-8");
    ssize_t ret = mbstowcs (_buf->base (), src, _buf->size ());
    if (ret < 0) {
      _err = true;
      _buf = NULL;
      _len = 0;
    } else {
      _len = ret;
      if (0 && src) {
	warn << "XX failed to completely convert string ('" 
	     << utf8_in << "'): " 
	     << "expected " << _len << " bytes, but only converted "
	     << (src - utf8_in.cstr ()) << "\n";
	_err = true;
      } else {
	_err = false;
      }
    }
  }
}

void
wide_str_t::init (const wchar_t *in, size_t l)
{
  init (l);
  memcpy (_buf->base (), in, _len* sizeof (wchar_t));
}

wide_str_t::wide_str_t (const wchar_t *in) { init (in, wcslen (in)); }
wide_str_t::wide_str_t (const wchar_t *in, size_t l) { init (in, l); }

str
wide_str_t::to_utf8 () const
{
  static size_t utf8_char_width = 6;
  str ret;
  if (_buf) {
    mbstate_t state;
    memset (&state, 0, sizeof (state));
    size_t len = _len * utf8_char_width;
    mstr m (len);
    const wchar_t *in = _buf->base ();
    ssize_t nbytes = wcsrtombs (m.cstr (), &in, len, &state);
    if (nbytes >= 0) {

      // should have consumed all bytes; if not '6' above is wrong!!
      assert (!in);

      m.setlen (nbytes);
      ret = m;
    }
  }
  return ret;
}

void
wide_str_t::setbuf (ptr<vec<wchar_t> > v, size_t sz)
{
  _buf = v;
  _len = sz;
  _err = false;
}

const wchar_t *
wide_str_t::buf (size_t *lenp) const
{
  const wchar_t *ret = NULL;
  if (_buf) { ret = _buf->base (); }
  if (lenp) { *lenp = _len ; }
  return ret;
}

wchar_t *
wide_str_t::buf (size_t *lenp) 
{
  wchar_t *ret = NULL;
  if (_buf) { ret = _buf->base (); }
  if (lenp) { *lenp = _len; }
  return ret;
}

void
wide_str_t::chop (size_t start, size_t len)
{
  if (_buf) {
    size_t n = min<size_t> (_len, start);
    _buf->popn_front (n);
    _len -= start;
    if (len < _len ) {
      (*_buf)[len] = wchar_t ('\0');
      _len = len;
    }
  }
}

const strbuf &
strbuf_cat (const strbuf &b, const wide_str_t &ws)
{
  str s = ws;
  if (s) b << s;
  return b;
}
