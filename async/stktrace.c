/* $Id$ */

/*
 *
 * Copyright (C) 2000 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "sysconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif /* DMALLOC */
#undef malloc
#undef free

#if __GNUC__ >= 2 && defined (__i386__)

static const char hexdigits[] = "0123456789abcdef";

struct traceback {
  struct traceback *next;
  char *name;
};

#define STKCACHESIZE 509U
static struct traceback *stkcache[STKCACHESIZE];

const char *
__backtrace (const char *file)
{
  const void *const *framep;
  size_t filelen;
  char buf[256];
  char *bp = buf + sizeof (buf);
  u_long bucket = 5381;
  struct traceback *tb;

  filelen = strlen (file);
  if (filelen >= sizeof (buf))
    return file;
  bp -= filelen + 1;
  strcpy (bp, file);

  __asm volatile ("movl %%ebp, %0" : "=g" (framep) :);
  while (framep[0] && bp >= buf + 9) {
    int i;
    u_long pc = (u_long) framep[1] - 1;
    framep = *framep;
    bucket = ((bucket << 5) + bucket) ^ pc;

    *--bp = ' ';
    *--bp = hexdigits[pc & 0xf];
    pc >>= 4;
    for (i = 0; pc && i < 7; i++) {
      *--bp = hexdigits[pc & 0xf];
      pc >>= 4;
    }
  }
  bucket = (bucket & 0xffffffff) % STKCACHESIZE;

  for (tb = stkcache[bucket]; tb; tb = tb->next)
    if (!strcmp (tb->name, bp))
      return tb->name;

  tb = malloc (sizeof (*tb));
  if (!tb)
    return file;
  tb->name = malloc (1 + strlen (bp));
  if (!tb->name) {
    free (tb);
    return file;
  }
  strcpy (tb->name, bp);
  tb->next = stkcache[bucket];
  stkcache[bucket] = tb;

  return tb->name;
}

#else /* !gcc 2 || !i386 */

const char *
__backtrace (const char *file)
{
  return file;
}

#endif /* !gcc 2 || !i386 */

#ifdef DMALLOC

#if DMALLOC_VERSION_MAJOR < 5
#define dmalloc_logpath _dmalloc_logpath
char *dmalloc_logpath;
#endif /* DMALLOC_VERSION_MAJOR < 5 */


static int stktrace_record;

const char *
stktrace (const char *file)
{
  if (stktrace_record < 0)
    return file;
  else if (!stktrace_record) {
    if (!dmalloc_logpath || !(dmalloc_debug_current () & 2)
	|| !getenv ("STKTRACE")) {
      stktrace_record = -1;
      return file;
    }
    stktrace_record = 1;
  }
  return __backtrace (file);
}

#endif /* DMALLOC */
