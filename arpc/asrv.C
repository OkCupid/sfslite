/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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


#include "arpc.h"

#ifdef MAINTAINER
int asrvtrace (getenv ("ASRV_TRACE") ? atoi (getenv ("ASRV_TRACE")) : 0);
bool asrvtime (getenv ("ASRV_TIME"));
#else /* !MAINTAINER */
enum { asrvtrace = 0, asrvtime = 0 };
#endif /* !MAINTAINER */

inline u_int32_t
xidswap (u_int32_t xid)
{
  return htonl (xid);
}

static inline const char *
tracetime ()
{
  static str buf ("");
  if (asrvtime) {
    timespec ts;
    clock_gettime (CLOCK_REALTIME, &ts);
    buf = strbuf (" %d.%06d", int (ts.tv_sec), int (ts.tv_nsec/1000));
  }
  return buf;
}

svccb::svccb ()
  : arg (NULL), aup (NULL), addr (NULL), addrlen (0),
    res (NULL), reslen (0), resdat (NULL)
{
  bzero (&msg, sizeof (msg));
}

svccb::~svccb ()
{
  xdr_free (reinterpret_cast<xdrproc_t> (xdr_callmsg), &msg);
  if (arg)
    xdr_delete (srv->tbl[proc ()].xdr_arg, arg);
  if (resdat)
    xdr_delete (srv->tbl[proc ()].xdr_res, resdat);
  if (aup)
    xdr_delete (reinterpret_cast<xdrproc_t> (xdr_authunix_parms), aup);
  if (srv)
    srv->xi->svcdel ();
  xfree (res);
  delete addr;
}

bool
svccb::operator== (const svccb &a) const
{
  return (xid () == a.xid () && prog () == a.prog ()
	  && vers () == a.vers () && proc () == a.proc ()
	  && addrlen == a.addrlen && !memcmp (addr, a.addr, addrlen));
}

u_int
svccb::hash_value () const
{
  return xid () ^ hash_bytes (addr, addrlen);
}

void
svccb::init (asrv *s, const sockaddr *src)
{
  srv = mkref (s);
  srv->xi->svcadd ();
  if (!s->xi->xh->connected) {
    addrlen = s->xi->xh->socksize;
    addr = (sockaddr *) opnew (addrlen);
    memcpy (addr, src, addrlen);
  }
}

void *
svccb::getvoidres ()
{
  if (!resdat)
    resdat = srv->tbl[proc ()].alloc_res ();
  return resdat;
}

const authunix_parms *
svccb::getaup () const
{
  if (aup || msg.rm_call.cb_cred.oa_flavor != AUTH_UNIX)
    return aup;
  xdrmem x (msg.rm_call.cb_cred.oa_base,
	    msg.rm_call.cb_cred.oa_length, XDR_DECODE);
  aup = New authunix_parms;
  bzero (aup, sizeof (*aup));
  if (xdr_authunix_parms (x.xdrp (), aup))
    return aup;
  xdr_free (reinterpret_cast<xdrproc_t> (xdr_authunix_parms), aup);
  delete aup;
  // msg.rm_call.cb_cred.oa_flavor = AUTH_NONE;
  return aup = NULL;
}

u_int32_t
svccb::getaui () const
{
  if (msg.rm_call.cb_cred.oa_flavor != AUTH_UINT
      || msg.rm_call.cb_cred.oa_length != 4)
    return 0;
  return ntohl (*(u_int32_t *) msg.rm_call.cb_cred.oa_base);
}

bool
svccb::fromresvport () const
{
  const sockaddr_in *sinp = reinterpret_cast<const sockaddr_in *> (getsa ());
  return (sinp && sinp->sin_family == AF_INET
	  && ntohs (sinp->sin_port) < IPPORT_RESERVED);
}

void
svccb::reply (const void *reply, xdrproc_t xdr, bool nocache)
{
  rpc_msg rm;

  rm.rm_xid = xid ();
  rm.rm_direction = REPLY;
  rm.rm_reply.rp_stat = MSG_ACCEPTED;
  rm.acpted_rply.ar_verf = _null_auth;
  rm.acpted_rply.ar_stat = SUCCESS;
  rm.acpted_rply.ar_results.where = (char *) reply;
  rm.acpted_rply.ar_results.proc
    = reinterpret_cast<sun_xdrproc_t> (xdr ? xdr : srv->tbl[proc ()].xdr_res);

  xdrsuio x (XDR_ENCODE);
  if (!xdr_replymsg (x.xdrp (), &rm)) {
    warn ("svccb::reply: xdr_replymsg failed\n");
    delete this;
    return;
  }

  if (asrvtrace >= 4)
    warn ("ASRV_TRACE:%s reply %s:%s x=%x\n", tracetime (),
	  srv->rpcprog->name, srv->tbl[msg.rm_call.cb_proc].name,
	  xidswap (msg.rm_xid));
  if (asrvtrace >= 5 && !xdr && srv->tbl[msg.rm_call.cb_proc].print_res)
    srv->tbl[msg.rm_call.cb_proc].print_res (reply, NULL, asrvtrace - 4,
					     "REPLY", "");
  srv->sendreply (this, &x, nocache);
}

static void
asrv_rpc_mismatch (ref<xhinfo> xi, const sockaddr *addr, u_int32_t xid)
{
  rpc_msg m;

  bzero (&m, sizeof (m));
  m.rm_xid = xid;
  m.rm_direction = REPLY;
  m.rm_reply.rp_stat = MSG_DENIED;
  m.rjcted_rply.rj_stat = RPC_MISMATCH;
  m.rjcted_rply.rj_vers.low = RPC_MSG_VERSION;
  m.rjcted_rply.rj_vers.high = RPC_MSG_VERSION;

  xdrsuio x (XDR_ENCODE);
  if (xdr_replymsg (x.xdrp (), &m))
    xi->xh->sendv (x.iov (), x.iovcnt (), addr);
  else
    warn ("asrv_rpc_mismatch: xdr_replymsg failed\n");
}

static void
asrv_auth_reject (ref<xhinfo> xi, const sockaddr *addr,
		  u_int32_t xid, auth_stat stat)
{
  rpc_msg m;

  bzero (&m, sizeof (m));
  assert (stat != AUTH_OK);
  m.rm_xid = xid;
  m.rm_direction = REPLY;
  m.rm_reply.rp_stat = MSG_DENIED;
  m.rjcted_rply.rj_stat = AUTH_ERROR;
  m.rjcted_rply.rj_why = stat;

  xdrsuio x (XDR_ENCODE);
  if (xdr_replymsg (x.xdrp (), &m))
    xi->xh->sendv (x.iov (), x.iovcnt (), addr);
  else
    warn ("asrv_auth_reject: xdr_replymsg failed\n");
}

static void
asrv_accepterr (ref<xhinfo> xi, const sockaddr *addr,
		accept_stat stat, const rpc_msg *mp)
{
  rpc_msg m;

  bzero (&m, sizeof (m));
  m.rm_xid = mp->rm_xid;
  m.rm_direction = REPLY;
  m.rm_reply.rp_stat = MSG_ACCEPTED;

  switch (stat) {
  case PROG_UNAVAIL:
  case PROG_MISMATCH:
    {
      m.acpted_rply.ar_stat = PROG_UNAVAIL;
      m.acpted_rply.ar_vers.low = 0xffffffff;
      m.acpted_rply.ar_vers.high = 0;

      u_int32_t prog = mp->rm_call.cb_prog;
      u_int32_t vers = mp->rm_call.cb_vers;
      for (asrv *a = xi->stab.first (); a; a = xi->stab.next (a))
	if (a->hascb () && a->pv.prog == prog) {
	  if (a->pv.vers == vers)
	    panic ("asrv_accepterr: prog/vers exists\n");
	    // m.acpted_rply.ar_stat = PROC_UNAVAIL;
	  else if (m.acpted_rply.ar_stat != PROC_UNAVAIL) {
	    m.acpted_rply.ar_stat = PROG_MISMATCH;
	    if (m.acpted_rply.ar_vers.low > a->pv.vers)
	      m.acpted_rply.ar_vers.low = a->pv.vers;
	    if (m.acpted_rply.ar_vers.high < a->pv.vers)
	      m.acpted_rply.ar_vers.high = a->pv.vers;
	  }
	}
      break;
    }
  case PROC_UNAVAIL:
  case GARBAGE_ARGS:
  case SYSTEM_ERR:
    m.acpted_rply.ar_stat = stat;
    break;
  default:
    panic ("asrv_accepterr: bad stat %d\n", stat);
    break;
  }

  xdrsuio x (XDR_ENCODE);
  if (xdr_replymsg (x.xdrp (), &m))
    xi->xh->sendv (x.iov (), x.iovcnt (), addr);
  else
    warn ("asrv_accepterr: xdr_replymsg failed\n");
}

void
svccb::reject (auth_stat stat)
{
  if (asrvtrace >= 3)
    warn ("ASRV_TRACE:%s reject (auth_stat %d) %s:%s x=%x\n", tracetime (),
	  stat, srv->rpcprog->name, srv->tbl[msg.rm_call.cb_proc].name,
	  xidswap (msg.rm_xid));

  if (!srv->xi->ateof ())
    asrv_auth_reject (srv->xi, addr, xid (), stat);
  srv->sendreply (this, NULL, true);
}

void
svccb::reject (accept_stat stat)
{
  if (asrvtrace >= 3)
    warn ("ASRV_TRACE:%s reject (accept_stat %d) %s:%s x=%x\n", tracetime (),
	  stat, srv->rpcprog->name, srv->tbl[msg.rm_call.cb_proc].name,
	  xidswap (msg.rm_xid));

  if (!srv->xi->ateof ())
    asrv_accepterr (srv->xi, addr, stat, &msg);
  srv->sendreply (this, NULL, true);
}

void
svccb::ignore ()
{
  // Drop a request on the floor
  srv->sendreply (this, NULL, true);
}

asrv::asrv (ref<xhinfo> xi, const rpc_program &pr, asrv_cb::ptr cb)
  : rpcprog (&pr), tbl (pr.tbl), nproc (pr.nproc), cb (cb), xi (xi),
    pv (pr.progno, pr.versno)
{
  xi->stab.insert (this);
}

asrv::~asrv ()
{
  xi->stab.remove (this);
}

const ref<axprt> &
asrv::xprt () const
{
  return xi->xh;
}

ptr<asrv>
asrv::alloc (ref<axprt> x, const rpc_program &pr, asrv_cb::ptr cb)
{
  ptr<xhinfo> xi = xhinfo::lookup (x);
  if (!xi)
    return NULL;
  if (xi->stab[progvers (pr.progno, pr.versno)])
    panic ("attempt to reregister %s on same transport\n", pr.name);
  if (x->reliable)
    return New refcounted<asrv> (xi, pr, cb);
  else
    return New refcounted<asrv_unreliable> (xi, pr, cb);
}

void
asrv::seteof (ref<xhinfo> xi, const sockaddr *src)
{
  asrv *s;
  ptr<asrv> sp;
  for (s = xi->stab.first (); s; s = xi->stab.next (s)) {
    sp = mkref (s);
    if (s->cb)
      (*s->cb) (NULL);
  }
}

void
asrv::sendreply (svccb *sbp, xdrsuio *x, bool)
{
  if (!xi->ateof () && x)
    xi->xh->sendv (x->iov (), x->iovcnt (), sbp->addr);
  delete sbp;
}

void
asrv::setcb (asrv_cb::ptr c)
{
  cb = c;
  if (cb && xi->ateof ())
    (*cb) (NULL);
}

void
asrv::dispatch (ref<xhinfo> xi, const char *msg, ssize_t len,
		const sockaddr *src)
{
  if (!msg || len < 8 || getint (msg + 4) != CALL) {
    seteof (xi, src);
    return;
  }
 
  xdrmem x (msg, len, XDR_DECODE);
  auto_ptr<svccb> sbp (New svccb);
  rpc_msg *m = &sbp->msg;

  if (!xdr_callmsg (x.xdrp (), m)) {
    if (asrvtrace >= 1)
      warn ("asrv::dispatch: xdr_callmsg failed\n");
    seteof (xi, src);
    return;
  }
  if (m->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
    if (asrvtrace >= 1)
      warn ("asrv::dispatch: bad RPC message version\n");
    asrv_rpc_mismatch (xi, src, m->rm_xid);
    return;
  }

  asrv *s = xi->stab[progvers (sbp->prog (), sbp->vers ())];
  if (!s || !s->cb) {
    if (asrvtrace >= 1) {
      if (s)
	warn ("asrv::dispatch: no callback for %s (proc = %u)\n",
	      s->rpcprog->name, sbp->proc ());
      else
	warn ("asrv::dispatch: invalid prog/vers %u/%u (proc = %u)\n",
	      (u_int) sbp->prog (), (u_int) sbp->vers (),
	      (u_int) sbp->proc ());
    }
    asrv_accepterr (xi, src, PROG_UNAVAIL, m);
    return;
  }
  sbp->init (s, src);

  if (sbp->proc () >= s->nproc) {
    if (asrvtrace >= 1)
      warn ("asrv::dispatch: invalid procno %s:%u\n",
	    s->rpcprog->name, (u_int) sbp->proc ());
    asrv_accepterr (xi, src, PROC_UNAVAIL, m);
    return;
  }

  if (s->isreplay (sbp.get ())) {
    if (asrvtrace >= 1)
      warn ("asrv::dispatch: replay %s:%s x=%x\n",
	    s->rpcprog->name, s->tbl[m->rm_call.cb_proc].name,
	    xidswap (m->rm_xid));
    return;
  }

  const rpcgen_table *rtp = &s->tbl[sbp->proc ()];
  sbp->arg = s->tbl[sbp->proc ()].alloc_arg ();
  if (!rtp->xdr_arg (x.xdrp (), sbp->arg)) {
    if (asrvtrace >= 1)
      warn ("asrv::dispatch: bad message %s:%s x=%x\n", s->rpcprog->name,
	    rtp->name, xidswap (m->rm_xid));
    asrv_accepterr (xi, src, GARBAGE_ARGS, m);
    s->sendreply (sbp.release (), NULL, true);
    return;
  }

  if (asrvtrace >= 2) {
    if (const authunix_parms *aup = sbp->getaup ())
      warn ("ASRV_TRACE:%s serve %s:%s x=%x u=%u\n", tracetime (),
	    s->rpcprog->name, rtp->name, xidswap (m->rm_xid), aup->aup_uid);
    else if (u_int32_t i = sbp->getaui ())
      warn ("ASRV_TRACE:%s serve %s:%s x=%x i=%u\n", tracetime (),
	    s->rpcprog->name, rtp->name, xidswap (m->rm_xid), i);
    else
      warn ("ASRV_TRACE:%s serve %s:%s x=%x\n", tracetime (),
	    s->rpcprog->name, rtp->name, xidswap (m->rm_xid));
  }
  if (asrvtrace >= 5 && rtp->print_arg)
    rtp->print_arg (sbp->arg, NULL, asrvtrace - 4, "ARGS", "");

  (*s->cb) (sbp.release ());
}

void
asrv_unreliable::delsbp (svccb *sbp)
{
  rtab.remove (sbp);
  rq.remove (sbp);
  rsize--;
  delete sbp;
}

asrv_unreliable::~asrv_unreliable ()
{
  rq.traverse (wrap (this, &asrv_unreliable::delsbp));
}

bool
asrv_unreliable::isreplay (svccb *sbp)
{
  svccb *osbp = rtab[*sbp];
  if (!osbp) {
    rtab.insert (sbp);
    return false;
  }
  if (osbp->res)
    xi->xh->send (osbp->res, osbp->reslen, osbp->addr);
  return true;
}

void
asrv_unreliable::sendreply (svccb *sbp, xdrsuio *x, bool nocache)
{
  if (!x) {
    rtab.remove (sbp);
    delete sbp;
    return;
  }

  if (sbp->arg) {
    xdr_delete (tbl[sbp->proc ()].xdr_arg, sbp->arg);
    sbp->arg = NULL;
  }

  if (!xi->ateof ()) {
    sbp->reslen = x->uio ()->resid ();
    sbp->res = suio_flatten (x->uio ());
    x->uio ()->clear ();
    xi->xh->send (sbp->res, sbp->reslen, sbp->addr);
  }

  if (sbp->resdat) {
    xdr_delete (tbl[sbp->proc ()].xdr_res, sbp->resdat);
    sbp->resdat = NULL;
  }

  if (nocache) {
    rtab.remove (sbp);
    delete sbp;
    return;
  }
  else {
    ref<asrv> hold = sbp->srv;	// Don't let this be freed
    sbp->srv = NULL;		// Decrement reference count on this
    rsize++;
    rq.insert_tail (sbp);
    while (rsize > maxrsize)
      delsbp (rq.first);
  }
}
