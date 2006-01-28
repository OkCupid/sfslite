#include "tame.h"
#include "rxx.h"

u_int count_newlines (str s)
{
  const char *end = s.cstr () + s.len ();
  u_int c = 0;
  for (const char *cp = s.cstr (); cp < end; cp++) {
    if (*cp == '\n')
      c++;
  }
  return c;
}

bool
outputter_t::init ()
{
  if (_outfn && _outfn != "-") {
    if ((_fd = open (_outfn.cstr (), O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0) {
      warn << "cannot open file for writing: " << _outfn << "\n";
      return false;
    }
  } else {
    _fd = 1;
  }
  return true;
}

void
outputter_t::start_output ()
{
  _lineno = 1;

  // switching from NONE to PASSTHROUGH
  switch_to_mode (OUTPUT_PASSTHROUGH);
}

void
outputter_t::output_line_number ()
{
  if (_output_xlate) {
    strbuf b;
    if (!_last_char_was_nl)
      b << "\n";
    b << "# " << _lineno << " \"" << _infn << "\"\n";
    _output_str (b, false);
  }
}

void
outputter_t::output_str (str s)
{
  if (_mode == OUTPUT_TREADMILL) {
    static rxx x ("\n");
    vec<str> v;
    split (&v, x, s);
    for (u_int i = 0; i < v.size (); i++) {
      output_line_number ();
      _output_str (v[i], true);
    }
  } else {
    _output_str (s, false);
    if (_mode == OUTPUT_PASSTHROUGH)
      _lineno += count_newlines (s);
  }
}

void
outputter_t::_output_str (str s, bool add_nl)
{
  _strs.push_back (s);
  _buf << s;
  if (add_nl) {
    _buf << "\n";
    _last_char_was_nl = true;
  } else {
    if (s && s.len () && s[s.len () - 1] == '\n') {
      _last_char_was_nl = true;
    } else {
      _last_char_was_nl = false;
    }
  }
}

void
outputter_t::flush ()
{
  _buf.tosuio ()->output (_fd);
}

outputter_t::~outputter_t ()
{
  if (_fd >= 0) {
    flush ();
    close (_fd); 
  }
}

output_mode_t 
outputter_t::switch_to_mode (output_mode_t m)
{
  output_mode_t old = _mode;
  if (m == _mode)
    return _mode;
  if (m == OUTPUT_PASSTHROUGH && _mode != OUTPUT_NONE) 
    output_line_number ();
  _mode = m;
  return old;
}
