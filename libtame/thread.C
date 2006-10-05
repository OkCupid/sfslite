
#include "tame_thread.h"
#include "async.h"

#ifdef HAVE_PTH
# include <pth.h>
#endif /* HAVE_PTH */

int threads_out;

static void tame_thread_yield ()
{
#ifdef HAVE_PTH
  pth_yield (NULL);
#else 
  panic ("no PTH package available\n");
#endif /* HAVE_PTH */
}

void
tame_thread_spawn (const char *loc, void * (*fn) (void *), void *arg)
{
#ifdef HAVE_PTH

  //warn << "thread spawn ....\n";
  pth_attr_t attr = pth_attr_new ();
  pth_attr_set (attr, PTH_ATTR_NAME, loc); 
  pth_attr_set (attr, PTH_ATTR_STACK_SIZE, 0x10000);

  // Must be joinable to work with our scheduling hacks.
  pth_attr_set (attr, PTH_ATTR_JOINABLE, TRUE);

  threads_out ++;
    
  pth_spawn (attr, fn, arg);

  pth_attr_destroy (attr);

#else 
  panic ("no PTH package available\n");
#endif /* HAVE_PTH */
  
}

void
tame_thread_exit ()
{
#ifdef HAVE_PTH
  --threads_out;
  pth_exit (NULL);
#else 
  panic ("no PTH package available\n");
#endif /* HAVE_PTH */
}

void
tame_thread_init ()
{
#ifdef HAVE_PTH
  threads_out = 0;
#endif /* HAVE_PTH */
}

