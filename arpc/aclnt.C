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
#include "list.h"
#include "backoff.h"
#include "xdr_suio.h"

#ifdef MAINTAINER
int aclnttrace (getenv ("ACLNT_TRACE")
		? atoi (getenv ("ACLNT_TRACE")) : 0);
bool aclnttime (getenv ("ACLNT_TIME"));
#else /* !MAINTAINER */
enum { aclnttrace = 0, aclnttime = 0 };
#endif /* !MAINTAINER */

AUTH *auth_none;
static tmoq<rpccb_unreliable, &rpccb_unreliable::tlink> rpctoq;

static inline const char *
tracetime ()
{
  static str buf (""); 
  if (aclnttime) {
    timespec ts;
    clock_gettime (CLOCK_REALTIME, &ts);
    buf = strbuf (" %d.%06d", int (ts.tv_sec), int (ts.tv_nsec/1000));
  }
  return buf;
}

static void
ignore_clnt_stat (clnt_stat)
{
}
aclnt_cb aclnt_cb_null (wrap (ignore_clnt_stat));

INITFN (aclnt_init);

static void
aclnt_init ()
{
  auth_none = authnone_create ();
}

callbase::callbase (ref<aclnt> c, u_int32_t xid, const sockaddr *d)
  : c (c), dest (d), tmo (NULL), xid (xid)
{
  c->calls.insert_head (this);
  c->xi->xidtab.insert (this);
}

callbase::~callbase ()
{
  callbase *XXX_gcc296_bug __attribute__ ((unused)) = c->calls.remove (this);
  if (tmo)
    timecb_remove (tmo);
  c->xi->xidtab.remove (this);
  tmo = reinterpret_cast<timecb_t *> (0xc5c5c5c5); // XXX - debugging
}

bool
callbase::checksrc (const sockaddr *src) const
{
  if (c->xi->xh->connected)
    return true;
  return addreq (src, dest, c->xi->xh->socksize);
}

void
callbase::timeout (time_t duration)
{
  if (tmo)
    panic ("timeout already set (%p)\n"
	   "  rpccb type: %s\n\n"
	   "*** PLEASE REPORT THIS PROBLEM TO sfs-dev@pdos.lcs.mit.edu ***\n",
	   tmo, typeid (*this).name ());
  if (timecb_t *t = timecb (time (NULL) + duration,
			    wrap (this, &callbase::expire)))
    tmo = t;
}

u_int32_t (*next_xid) () = arandom;
static u_int32_t
genxid (xhinfo *xi)
{
  u_int32_t xid;
  while (xi->xidtab[xid = (*next_xid) ()] || !xid)
    ;
  return xid;
}

rpccb::rpccb (ref<aclnt> c, u_int32_t xid, aclnt_cb cb,
	      void *out, xdrproc_t outproc, const sockaddr *d)
  : callbase (c, xid, d), cb (cb), outmem (out), outxdr (outproc)
{
  assert (!tmo);
}

rpccb::rpccb (ref<aclnt> c, xdrsuio &x, aclnt_cb cb,
	      void *out, xdrproc_t outproc, const sockaddr *d, bool doxmit)
  : callbase (c, getxid (x), d), cb (cb), outmem (out), outxdr (outproc)
{
  assert (!tmo);
  if (doxmit)
    c->xprt ()->sendv (x.iov (), x.iovcnt (), d);
  if (tmo && !c->xprt ()->reliable) {
    panic ("transport callback added timeout (%p)\n"
	   "   xprt type: %s\n  rpccb type: %s\n\n"
	   "*** PLEASE REPORT THIS PROBLEM TO sfs-dev@pdos.lcs.mit.edu ***\n",
	   tmo, typeid (*c->xprt ()).name (),
	   typeid (*this).name ());
  }
}

void
rpccb::finish (clnt_stat stat)
{
  aclnt_cb c (cb);
  delete this;
  (*c) (stat);
}

u_int32_t
rpccb::getxid (xdrsuio &x)
{
  assert (x.iovcnt () > 0);
  assert (x.iov ()[0].iov_len >= 4);
  u_int32_t *xidp = reinterpret_cast<u_int32_t *> (x.iov ()[0].iov_base);
  if (!*xidp)
    *xidp = genxid (c->xi);
  return *xidp;
}

u_int32_t
rpccb::getxid (char *buf, size_t len)
{
  assert (len >= 4);
  u_int32_t *xidp = reinterpret_cast<u_int32_t *> (buf);
  if (!*xidp)
    *xidp = genxid (c->xi);
  return *xidp;
}

clnt_stat
rpccb::decodemsg (const char *msg, size_t len)
{
  char *m = const_cast<char *> (msg);

  xdrmem x (m, len, XDR_DECODE);
  rpc_msg rm;
  bzero (&rm, sizeof (rm));
  rm.acpted_rply.ar_verf = _null_auth; 
  rm.acpted_rply.ar_results.where = (char *) outmem;
  rm.acpted_rply.ar_results.proc = reinterpret_cast<sun_xdrproc_t> (outxdr);
  bool ok = xdr_replymsg (x.xdrp (), &rm);

  /* We don't support any auths with meaningful reply verfs */
  if (rm.rm_direction == REPLY && rm.rm_reply.rp_stat == MSG_ACCEPTED
      && rm.acpted_rply.ar_verf.oa_base)
    free (rm.acpted_rply.ar_verf.oa_base);

  if (!ok)
    return RPC_CANTDECODERES;
  rpc_err re;
  seterr_reply (&rm, &re);
  return re.re_status;
}

void
rpccb_msgbuf::xmit (int retry)
{
  if (aclnttrace >= 2 && retry > 0) {
    warn ("ACLNT_TRACE:%s retransmit #%d x=%x\n", tracetime (), retry,
	  *reinterpret_cast<u_int32_t *> (msgbuf));
  }
  c->xprt ()->send (msgbuf, msglen, dest);
}

rpccb_unreliable::rpccb_unreliable (ref<aclnt> c, xdrsuio &x,
				    aclnt_cb cb,
				    void *out, xdrproc_t outproc,
				    const sockaddr *d)
  : rpccb_msgbuf (c, x, cb, out, outproc, d)
{
  assert (!tmo);
  rpctoq.start (this);
  assert (!tmo);
}

rpccb_unreliable::rpccb_unreliable (ref<aclnt> c,
				    char *buf, size_t len,
				    aclnt_cb cb,
				    void *out, xdrproc_t outproc,
				    const sockaddr *d)
  : rpccb_msgbuf (c, buf, len, cb, out, outproc, d)
{
  assert (!tmo);
  rpctoq.start (this);
  assert (!tmo);
}

rpccb_unreliable::~rpccb_unreliable ()
{
  rpctoq.remove (this);
}

aclnt::aclnt (const ref<xhinfo> &x, const rpc_program &p)
  : xi (x), rp (p), eofcb (NULL)
{
  xi->clist.insert_head (this);
}

aclnt::~aclnt ()
{
  assert (!calls.first);
  aclnt *XXX_gcc296_bug __attribute__ ((unused)) = xi->clist.remove (this);
  xfree (dest);
}

const ref<axprt> &
aclnt::xprt () const
{
  return xi->xh;
}

bool
aclnt::marshal_call (xdrsuio &x, AUTH *auth,
		     u_int32_t progno, u_int32_t versno, u_int32_t procno,
		     xdrproc_t inproc, const void *in)
{
  u_int32_t *dp = (u_int32_t *) XDR_INLINE (x.xdrp (), 6*4);
#if 0
  if (dp) {
#endif
    *dp++ = 0;
    *dp++ = htonl (CALL);
    *dp++ = htonl (RPC_MSG_VERSION);
    *dp++ = htonl (progno);
    *dp++ = htonl (versno);
    *dp++ = htonl (procno);
#if 0
  }
  else {
    xdr_putint (x.xdrp (), 0);
    xdr_putint (x.xdrp (), CALL);
    xdr_putint (x.xdrp (), RPC_MSG_VERSION);
    xdr_putint (x.xdrp (), progno);
    xdr_putint (x.xdrp (), versno);
    xdr_putint (x.xdrp (), procno);
  }
#endif
  if (!AUTH_MARSHALL (auth ? auth : auth_none, x.xdrp ())) {
    warn ("failed to marshal auth crap\n");
    return false;
  }
  if (!inproc (x.xdrp (), const_cast<void *> (in))) {
    warn ("arg marshaling failed (prog %d, vers %d, proc %d)\n",
	  progno, versno, procno);
    return false;
  }
  return true;
}

static void
printreply (aclnt_cb cb, str name, void *res,
	    void (*print_res) (const void *, const strbuf *, int,
			       const char *, const char *),
	    clnt_stat err)
{
  if (aclnttrace >= 3) {
    if (err)
      warn << "ACLNT_TRACE:" << tracetime () 
	   << " reply " << name << ": " << err << "\n";
    else if (aclnttrace >= 4) {
      warn << "ACLNT_TRACE:" << tracetime ()
	   << " reply " << name << "\n";
      if (aclnttrace >= 5 && print_res)
	print_res (res, NULL, aclnttrace - 4, "REPLY", "");
    }
  }
  (*cb) (err);
}

bool aclnt::init_call (xdrsuio &x,
		       u_int32_t procno, const void *in, void *out,
		       aclnt_cb &cb, AUTH *auth,
		       xdrproc_t inproc, xdrproc_t outproc,
		       u_int32_t progno, u_int32_t versno)
{
  if (xi->ateof ()) {
    (*cb) (RPC_CANTSEND);
    return false;
  }

  if (!auth)
    auth = auth_none;
  if (!progno) {
    progno = rp.progno;
    assert (procno < rp.nproc);
    if (!inproc)
      inproc = rp.tbl[procno].xdr_arg;
    if (!outproc)
      outproc = rp.tbl[procno].xdr_res;
    if (!versno)
      versno = rp.versno;
  }
  assert (inproc);
  assert (outproc);
  assert (progno);
  assert (versno);

  if (!marshal_call (x, auth, progno, versno, procno, inproc, in)) {
    (*cb) (RPC_CANTENCODEARGS);
    return false;
  }
  assert (x.iov ()[0].iov_len >= 4);
  u_int32_t &xid = *reinterpret_cast<u_int32_t *> (x.iov ()[0].iov_base);
  if (!xi->xh->reliable || cb != aclnt_cb_null)
    xid = genxid (xi);

  if (aclnttrace >= 2) {
    str name;
    const rpcgen_table *rtp;
    if (progno == rp.progno && versno == rp.versno && procno < rp.nproc) {
      rtp = &rp.tbl[procno];
      name = strbuf ("%s:%s x=%x", rp.name, rtp->name, xid);
    }
    else {
      rtp = NULL;
      name = strbuf ("prog %d vers %d proc %d x=%x",
		     progno, versno, procno, xid);
    }
    warn << "ACLNT_TRACE:" << tracetime () << " call " << name << "\n";
    if (aclnttrace >= 5 && rtp && rtp->xdr_arg == inproc && rtp->print_arg)
      rtp->print_arg (in, NULL, aclnttrace - 4, "ARGS", "");
    if (aclnttrace >= 3 && cb != aclnt_cb_null)
      cb = wrap (printreply, cb, name, out,
		 (rtp && rtp->xdr_res == outproc ? rtp->print_res : NULL));
  }

  return true;
}

callbase *
aclnt::call (u_int32_t procno, const void *in, void *out,
	     aclnt_cb cb,
	     AUTH *auth,
	     xdrproc_t inproc, xdrproc_t outproc,
	     u_int32_t progno, u_int32_t versno,
	     sockaddr *d)
{
  xdrsuio x (XDR_ENCODE);
  if (!init_call (x, procno, in, out, cb, auth, inproc,
		  outproc, progno, versno))
    return NULL;
  if (!outproc)
    outproc = rp.tbl[procno].xdr_res;
  if (!d)
    d = dest;
  if (xi->xh->reliable && cb == aclnt_cb_null) {
    /* If we don't care about the reply, then don't bother keeping
     * state around.  We use the reserved XID 0 for these calls, so
     * that we don't accidentally recycle the XID of a call whose
     * state we threw away. */
    xi->xh->sendv (x.iov (), x.iovcnt (), d);
    return NULL;
  }
  else {
    callbase *ret = (*rpccb_alloc) (mkref (this), x, cb, out, outproc, d);
    if (xi->xh->ateof ())
      return NULL;
    return ret;
  }
}

callbase *
aclnt::timedcall (time_t duration, u_int32_t procno, const void *in, void *out,
		  aclnt_cb cb,
		  AUTH *auth,
		  xdrproc_t inproc, xdrproc_t outproc,
		  u_int32_t progno, u_int32_t versno,
		  sockaddr *d)
{
  callbase *cbase = call (procno, in, out, cb, auth, inproc,
			  outproc, progno, versno, d);
  if (cbase)
    cbase->timeout (duration);
  return cbase;
}

static void
scall_cb (clnt_stat *errp, bool *donep, clnt_stat err)
{
  *errp = err;
  *donep = true;
}

clnt_stat
aclnt::scall (u_int32_t procno, const void *in, void *out,
	      AUTH *auth,
	      xdrproc_t inproc, xdrproc_t outproc,
	      u_int32_t progno, u_int32_t versno,
	      sockaddr *d, time_t duration)
{
  bool done = false;
  clnt_stat err;
  callbase *cbase = call (procno, in, out, wrap (scall_cb, &err, &done),
			  auth, inproc, outproc, progno, versno, d);
  if (cbase && duration)
    cbase->timeout (duration);
  while (!done)
    acheck ();
  return err;
}

class rawcall : public callbase {
  aclntraw_cb::ptr cb;
  u_int32_t oldxid;

  rawcall ();

  PRIVDEST virtual ~rawcall () {}

public:
  rawcall (ref<aclnt> c, const char *msg, size_t len,
	   aclntraw_cb cb, sockaddr *d)
    : callbase (c, genxid (c->xi), d), cb (cb) {
    assert (len >= 4);
    assert (c->xprt ()->reliable);
    memcpy (&oldxid, msg, 4);
    iovec iov[2] = {
      { iovbase_t (&xid), 4 },
      { iovbase_t (msg + 4), len - 4 },
    };
    c->xprt ()->sendv (iov, 2, d);
  }

  clnt_stat decodemsg (const char *msg, size_t len) {
    memcpy (const_cast<char *> (msg), &oldxid, 4);
    (*cb) (RPC_SUCCESS, msg, len);
    cb = NULL;
    return RPC_SUCCESS;
  }
  void finish (clnt_stat stat) {
    if (cb)
      (*cb) (stat, NULL, -1);
    delete this;
  }
};

callbase *
aclnt::rawcall (const char *msg, size_t len, aclntraw_cb cb, sockaddr *dest)
{
  return New ::rawcall (mkref (this), msg, len, cb, dest);
}

void
aclnt::seteofcb (cbv::ptr e)
{
  eofcb = e;
  if (xi->ateof ()) {
    eofcb = NULL;
    (*e) ();
  }
}

inline ptr<aclnt>
aclnt_mkptr (aclnt *c)
{
  if (c)
    return mkref (c);
  else
    return NULL;
}

void
aclnt::seteof (ref<xhinfo> xi)
{
  ptr<aclnt> c;
  if (xi->xh->connected)
    for (c = aclnt_mkptr (xi->clist.first); c;
	 c = aclnt_mkptr (xi->clist.next (c))) {
      callbase *rb, *nrb;
      for (rb = c->calls.first; rb; rb = nrb) {
	nrb = c->calls.next (rb);
	rb->finish (RPC_CANTRECV);
      }
      if (c->eofcb)
	(*c->eofcb) ();
    }
}

void
aclnt::dispatch (ref<xhinfo> xi, const char *msg, ssize_t len,
		 const sockaddr *src)
{
  if (!msg || len < 8 || getint (msg + 4) != REPLY) {
    seteof (xi);
    return;
  }

  u_int32_t xid;
  memcpy (&xid, msg, sizeof (xid));
  callbase *rp = xi->xidtab[xid];
  if (!rp || !rp->checksrc (src))
    return;

  rp->finish (rp->decodemsg (msg, len));
}

ptr<aclnt>
aclnt::alloc (ref<axprt> x, const rpc_program &pr, const sockaddr *d,
	      aclnt::rpccb_alloc_t ra)
{
  ptr<xhinfo> xi = xhinfo::lookup (x);
  if (!xi)
    return NULL;
  ref<aclnt> c = New refcounted<aclnt> (xi, pr);
  if (!x->connected && d) {
    c->dest = (sockaddr *) xmalloc (x->socksize);
    memcpy (c->dest, d, x->socksize);
  }
  else
    c->dest = NULL;
  if (ra)
    c->rpccb_alloc = ra;
  else if (xi->xh->reliable)
    c->rpccb_alloc = callbase_alloc<rpccb>;
  else
    c->rpccb_alloc = callbase_alloc<rpccb_unreliable>;
  return c;
}
