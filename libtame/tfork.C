
#include "tame_tfork.h"

void __tfork (const char *loc, evv_t e, cbv a)
{
  cthread_t<void> *t = New cthread_t<void> (e, a);
  tame_thread_spawn (loc, cthread_t<void>::run, static_cast<void *> (t)); 
}

void _tfork (implicit_rendezvous_t *i, const char *loc, cbv a)
{
  __tfork (loc, _mkevent (i, loc), a);
}

void _tfork (ptr<closure_t> c, const char *loc, rendezvous_t<> rv, cbv a)
{
  __tfork (loc, _mkevent (c, loc, rv), a);
}
