/* $Id$ */

#include "amisc.h"

#ifdef DMALLOC
bool dmalloc_init::initialized;
void
dmalloc_init::init ()
{
  if (suidsafe () < 0
      && (getenv ("DMALLOC_OPTIONS") || getenv ("STKTRACE"))) {
    setgid (getgid ());
    setuid (getuid ());
    const char msg[] = "setuid disabled for malloc debugging\n";
    write (2, msg, sizeof (msg) - 1);
  }
  initialized = true;
}
#endif /* DMALLOC */

/* The xmalloc handler will have no effect when dmalloc is used, but
 * we leave it in for compatibility. */
static void
default_xmalloc_handler (int size)
{
  const char msg[] = "malloc failed\n";
  write (errfd, msg, sizeof (msg) - 1);
  abort ();
}
void (*xmalloc_handler) (int) = default_xmalloc_handler;

#ifdef DMALLOC

#undef new
dmalloc_t dmalloc;

void *
operator new (size_t size, dmalloc_t, const char *file, int line)
{
  /* As per the C++ standard, allocating 0 bytes must not return NULL,
   * and must return a different pointer each time... */
  if (!size)
    size = 1;
  return _xmalloc_leap (file, line, size);
}

void *
operator new[] (size_t size, dmalloc_t, const char *file, int line)
{
  if (!size)
    size = 1;
  return _xmalloc_leap (file, line, size);
}

void *
operator new (size_t size, nothrow_t, const char *file, int line) throw ()
{
  if (!size)
    size = 1;
  return _malloc_leap (file, line, size);
}

void *
operator new[] (size_t size, nothrow_t, const char *file, int line) throw ()
{
  if (!size)
    size = 1;
  return _malloc_leap (file, line, size);
}

#else /* !DMALLOC */

void *
xmalloc (size_t size)
{
  void *p;
  if (!(p = malloc (size)))
    default_xmalloc_handler (size);
  return p;
}

void *
xrealloc (void *o, size_t size)
{
  void *p;
  if (!(p = realloc (o, size)))
    default_xmalloc_handler (size);
  return p;
}

char *
xstrdup (const char *s)
{
  char *d;
  d = (char *) xmalloc (strlen (s) + 1);
  strcpy (d, s);
  return d;
}

#endif /* !DMALLOC */

using std::bad_alloc;
using std::nothrow_t;

void *
operator new (size_t size) throw (bad_alloc)
{
  if (!size)
    size = 1;
  return txmalloc (size);
}

void *
operator new[] (size_t size) throw (bad_alloc)
{
  if (!size)
    size = 1;
  return txmalloc (size);
}

void *
operator new (size_t size, nothrow_t) throw ()
{
  if (!size)
    size = 1;
  return malloc (size);
}

void *
operator new[] (size_t size, nothrow_t) throw ()
{
  if (!size)
    size = 1;
  return malloc (size);
}

#ifndef delete_throw
# define delete_throw throw()
#endif /* !delete_throw */

void
operator delete (void *ptr) delete_throw
{
  xfree (ptr);
}

void
operator delete[] (void *ptr) delete_throw
{
  xfree (ptr);
}
