
// -*-c++-*-

#ifndef __PY_DEBUG_H_INCLUDED__
#define __PY_DEBUG_H_INCLUDED__

#include "qhash.h"
#include "async.h"

typedef qhash<str,int> debug_dict_t;

template<class W> void pydebug_inc (debug_dict_t *d);
template<class W> void pydebug_dec (debug_dict_t *d);
template<class W> str getkey ();

// XXX temporary
#define PYDEBUG 1

#ifdef PYDEBUG
void pydebug_memreport (const strbuf &b);
extern debug_dict_t g_new_cnt, g_del_cnt;

# define PYDEBUG_MEMREPORT(b) pydebug_memreport (b)
# define PYDEBUG_VIRTUAL_DESTRUCTOR virtual

# define PYDEBUG_NEW(W)                \
   _pydebug_free = false;              \
   pydebug_inc<W> (&g_new_cnt);        

# define PYDEBUG_PYALLOC(T) pydebug_inc (#T, &g_new_cnt);
# define PYDEBUG_PYFREE(T)  pydebug_inc (#T, &g_del_cnt);

# define PYDEBUG_CLASS_MEMBERS         \
   bool _pydebug_free;

# define PYDEBUG_DEL(W)                \
   pydebug_del ();

# define PYDEBUG_DEL_FUNC(obj)         \
  obj.pydebug_del ();

# define PYDEBUG_CLASS_METHODS         \
inline void pydebug_del() {            \
   if (!_pydebug_free) {               \
      pydebug_inc<W> (&g_del_cnt);     \
       _pydebug_free = true;           \
   }                                   \
}


#else /* ! PYDEBUG */
# define PYDEBUG_NEW(W) 
# define PYDEBUG_DEL(W) 
# define PYDEBUG_PYALLOC(T)
# define PYDEBUG_PYFREE(T)
# define PYDEBUG_MEMREPORT()
# define PYDEBUG_DEL_FUNC(O)
# define PYDEBUG_VIRTUAL_DESTRUCTOR 
# define PYDEBG_CLASS_MEMBERS
# define PYDEBUG_CLASS_METHODS
#endif /* PYDEBUG */

//-----------------------------------------------------------------------
//
// More Debug Stuff
//

template<class W> void
pydebug_inc (qhash<str,int> *hsh)
{
  str k = getkey<W> ();
  if (!k) return;
  pydebug_inc (k, hsh);
}

void pydebug_inc (const str &k, qhash<str,int> *hsh);
void pydebug_dec (const str &k, qhash<str,int> *hsh);

template<class W> void
pydebug_dec (qhash<str,int> *hsh)
{
  str k = getkey<W> ();
  if (!k) return;
  pydebug_dec (k, hsh);
}

//
//
//-----------------------------------------------------------------------



#endif /* __PY_DEBUG_H_INCLUDED__ */


