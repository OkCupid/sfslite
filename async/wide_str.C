#include "wide_str.h"
#include <locale.h>
#include "err.h"

bool
wide_str_t::init (const char *locale)
{
  return setlocale (LC_CTYPE, locale);
}

void
wide_str_t::_init (size_t l)
{
  _buf = New refcounted<vec<wchar_t> > ();
  _buf->setsize (l+1);
  memset (_buf->base (), 0, sizeof (wchar_t) * (l+1));
  _len = l;
  _err = false;
}

wide_str_t::wide_str_t (str utf8_in)
{
  if (utf8_in) {
    _init (utf8_in.len ());
    mbstate_t state;
    memset (&state, 0, sizeof (state));
    const char *src = utf8_in.cstr ();
    setlocale (LC_CTYPE, "en_US.UTF-8");
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
wide_str_t::_init (const wchar_t *in, size_t l)
{
  _init (l);
  memcpy (_buf->base (), in, _len* sizeof (wchar_t));
}

wide_str_t::wide_str_t (const wchar_t *in) { _init (in, wcslen (in)); }
wide_str_t::wide_str_t (const wchar_t *in, size_t l) { _init (in, l); }

str
wide_str_t::to_utf8 () const
{
  static size_t utf8_char_width = 6;
  str ret;
  if (!_buf) { /*noop */ } 
  else if (_len == 0) { ret = ""; }
  else {
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
    _len -= n;
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

wide_str_t
wide_str_t::substr (size_t start, size_t len) const
{
  wide_str_t ret (*this);
  ret.chop (start, len);
  return ret;
}

str
utf8_substr (const str &s, size_t pos, size_t len)
{
  wide_str_t ws (s);
  ws.chop (pos, len);
  return ws.to_utf8 ();
}

size_t point_width (unsigned char ch)
{
  struct { 
    unsigned char top;
    int ret;
  } prefixes[] = {
    { 0x7f, 1 },
    { 0xbf, -1 },
    { 0xc1, 2 },
    { 0xdf, 2 },
    { 0xef, 3 },
    { 0xf4, 4 },
    { 0xf7, 4 },
    { 0xfb, 5 },
    { 0xfd, 6 },
    { 0xff, -1 }
  };

  for (size_t i = 0; i < sizeof (prefixes) / sizeof (prefixes[0]); i++) {
    if (ch <= prefixes[i].top) return prefixes[i].ret;
  }
  return 0;
}

bool is_follow_byte (unsigned char ch)
{
  return (ch >= 0x80 && ch <= 0xbf);
}

static void add_repl (char *&op, const str &repl)
{
  if (!repl || !repl.len ()) return;
  for (const char *rp = repl.cstr (); *rp; rp++) { *op++ = *rp; } 
}

str 
utf8_fix (const str &in, const str &repl)
{
  if (!in) return in;

  size_t repl_len = repl ? repl.len () : size_t (0);
  mstr out (in.len () * max<size_t> (1, repl_len) + 1);
  const char *ip = in.cstr ();
  const char *endp = ip + in.len ();
  char *op = out.cstr ();
  const char *cps = NULL; // code point start
  ssize_t expected_width = 0, tmp = 0;

  for ( ; ip < endp; ip++) {

    bool consumed = false;

    // if 'cps' is true, that means we've previously started
    // a code point
    if (cps) {

      // We're expecting a follow byte -- if there's none here, it's
      // an error and we need to throw away our buffered code point.
      if (!is_follow_byte (*ip)) { 
	cps = NULL; 
	expected_width = 0;
	add_repl (op, repl);

	// We do have a follow byte, and we've seen enough bytes,
	// so we're all set.
      } else if (ip - cps == expected_width - 1) {
	while (cps <= ip) {
	  *op++ = *cps++;
	}
	cps = NULL;
	expected_width = 0;
	consumed = true;
      } else {
	// in the default case, we haven't seen enough data, so keep moving.
	consumed = true;
      }

    }

    // Need to handle the non-code-point case AFTER the code-point case,
    // since in some cases (a busted code-point), we hit both this if
    // and the one above.
    if (!cps && !consumed) { 
      tmp = point_width (*ip);
      // start a new code point
      if (tmp > 1) {
	cps = ip;
	expected_width = tmp;
      } else if (tmp == 1) {
	*op++ = *ip;
      } else {
	// we hit a bad starting byte, or a follow byte, neither of which
	// should be useful here.
	add_repl (op, repl);
      }
    }

  }

  out.setlen (op - out.cstr ());
  return out;
}
