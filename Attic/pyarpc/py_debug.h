
// -*-c++-*-

#ifndef __PY_DEBUG_H_INCLUDED__
#define __PY_DEBUG_H_INCLUDED__

#include "qhash.h"
#include "async.h"

typedef qhash<str,int> debug_dict_t;

template<class W> void pydebug_inc (debug_dict_t *d);
template<class W> void pydebug_dec (debug_dict_t *d);

// XXX temporary
#define PYDEBUG 1

#ifdef PYDEBUG
void pydebug_memreport (const strbuf &b);
extern debug_dict_t g_new_cnt, g_del_cnt;
# define PYDEBUG_NEW(W) pydebug_inc<W> (&g_new_cnt);
# define PYDEBUG_DEL(W) pydebug_inc<W> (&g_del_cnt);
# define PYDEBUG_MEMREPORT(b) pydebug_memreport (b)
# define PYDEBUG_VIRTUAL_DESTRUCTOR virtual

#else /* ! PYDEBUG */
# define PYDEBUG_NEW(W) 
# define PYDEBUG_DEL(W) 
# define PYDEBUG_MEMREPORT()
# define PYDEBUG_VIRTUAL_DESTRUCTOR 
#endif /* PYDEBUG */



#endif /* __PY_DEBUG_H_INCLUDED__ */


