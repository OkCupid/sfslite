
// -*-c++-*-
/* $Id: core.C 2654 2007-03-31 05:42:21Z max $ */

#include "tame_run.h"

void 
tame_error (const char *loc, const char *msg)
{
  if (!(tame_options & TAME_ERROR_SILENT)) {
    if (loc) {
      warn << loc << ": " << msg << "\n";
    } else 
      warn << msg << "\n";
  }
  if (tame_options & TAME_ERROR_FATAL)
    panic ("abort on tame failure\n");
}
