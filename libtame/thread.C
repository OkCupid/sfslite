
#include "tame_thread.h"
#include "async.h"

int threads_out;

void
tame_thread_spawn (const char *loc, void * (*fn) (void *), void *arg)
{
  panic ("no PTH package available\n");
}

void
tame_thread_exit ()
{
  panic ("no PTH package available\n");
}

void
tame_thread_init ()
{
}

