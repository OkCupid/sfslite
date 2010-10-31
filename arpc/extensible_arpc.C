
#include "xdrmisc.h"
#include "extensible_arpc.h"

//=======================================================================

ptr<v_XDR_dispatch_t> v_XDR_dispatch;

//-----------------------------------------------------------------------

v_XDR_t::~v_XDR_t () { m_dispatch->remove (m_x); }

//-----------------------------------------------------------------------

v_XDR_t::v_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x)
  : m_dispatch (d), m_x (x)
{
  m_dispatch->add (this);
}

//-----------------------------------------------------------------------

void v_XDR_dispatch_t::remove (XDR *x) 
{ m_tab.remove (key (x)); }

//-----------------------------------------------------------------------

void v_XDR_dispatch_t::add (v_XDR_t *x) 
{ m_tab.insert (key (x->xdrp ()), x); }

//-----------------------------------------------------------------------

ptr<v_XDR_t> v_XDR_dispatch_t::lookup (XDR *x)
{ 
  v_XDR_t **retp;
  ptr<v_XDR_t> ret;
  if ((retp = m_tab[key (x)])) {
    ret = mkref (*retp);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<v_XDR_t> xdr_virtualize (XDR *x)
{
  ptr<v_XDR_t> ret;
  if (v_XDR_dispatch) { ret = v_XDR_dispatch->lookup (x); }
  return ret;
}

//-----------------------------------------------------------------------

ptr<v_XDR_t> xdr_virtual_map (u_int32_t key, XDR *x)
{
  ptr<v_XDR_t> ret;
  if (v_XDR_dispatch) {
    ret = v_XDR_dispatch->alloc (key, x);
  } 
  return ret;
}

//-----------------------------------------------------------------------

uintptr_t v_XDR_dispatch_t::key (const XDR *v)
{ return reinterpret_cast<uintptr_t> (v); }

//=======================================================================
