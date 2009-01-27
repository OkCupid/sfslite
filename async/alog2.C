#include "alog2.h"

//-----------------------------------------------------------------------

void
logger2_t::log (log2_level_t l, const char *fmt, ...)
{
  if (!silent (l)) {
    va_list ap;
    va_start (ap, fmt);
    vwarn (fmt, ap);
    va_end (ap);
  }
}

//-----------------------------------------------------------------------

void
logger2_t::logx (log2_level_t l, const char *fmt, ...)
{
  if (!silent (l)) {
    va_list ap;
    va_start (ap, fmt);
    vwarnx (fmt, ap);
    va_end (ap);
  }
}

//-----------------------------------------------------------------------

void
logger2_t::log (log2_level_t l, const str &s) { if (!silent (l)) warn << s; }

//-----------------------------------------------------------------------

void
logger2_t::logx (log2_level_t l, const str &s) { if (!silent (l)) warnx << s; }

//-----------------------------------------------------------------------

logger2_t::obj_t
logger2_t::log (log2_level_t l) { return obj_t (silent (l), false); }

//-----------------------------------------------------------------------

logger2_t::obj_t 
logger2_t::logx (log2_level_t l) { return obj_t (silent (l), true); }

//-----------------------------------------------------------------------
