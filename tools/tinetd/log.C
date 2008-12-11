
#include "tinetd.h"



//-----------------------------------------------------------------------

logger_t logger;

//-----------------------------------------------------------------------

void
logger_t::log (level_t l, const char *fmt, ...)
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
logger_t::logx (level_t l, const char *fmt, ...)
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
logger_t::log (level_t l, const str &s) { if (!silent (l)) warn << s; }

//-----------------------------------------------------------------------

void
logger_t::logx (level_t l, const str &s) { if (!silent (l)) warnx << s; }

//-----------------------------------------------------------------------

logger_t::obj_t
logger_t::log (level_t l) { return obj_t (silent (l), false); }

//-----------------------------------------------------------------------

logger_t::obj_t
logger_t::logx (level_t l) { return obj_t (silent (l), true); }

//-----------------------------------------------------------------------
