
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

void
xdr_virtual_asrv_alloc (ptr<axprt> x)
{
  if (v_XDR_dispatch) {
    v_XDR_dispatch->v_asrv_alloc (x);
  }
}

//-----------------------------------------------------------------------

uintptr_t v_XDR_dispatch_t::key (const XDR *v)
{ return reinterpret_cast<uintptr_t> (v); }

//-----------------------------------------------------------------------

static vec<rpc_constant_collect_hook_t> rpc_cch_vec;

rpc_add_cch_t::rpc_add_cch_t (rpc_constant_collect_hook_t h)
{
  rpc_cch_vec.push_back (h);
}

//-----------------------------------------------------------------------

ptr<rpc_constant_collect_hooks_t>
fetch_rpc_constant_collect_hooks ()
{
  static ptr<rpc_constant_collect_hooks_t> ret;
  if (!ret) {
    ret = New refcounted<rpc_constant_collect_hooks_t> ();
    bhash<uintptr_t> h;
    for (size_t i = 0; i < rpc_cch_vec.size (); i++) {
      uintptr_t hk = reinterpret_cast<uintptr_t> (rpc_cch_vec[i]);
      if (!h[hk]) {
	ret->push_back (rpc_cch_vec[i]);
	h.insert (hk);
      }
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

void
global_rpc_constant_collect (rpc_constant_collector_t *rcc)
{
  ptr<rpc_constant_collect_hooks_t> h = fetch_rpc_constant_collect_hooks ();
  for (size_t i = 0; i < h->size (); i++) { (*(*h)[i])(rcc); }
}

//=======================================================================
