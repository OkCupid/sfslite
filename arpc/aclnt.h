// -*-c++-*-
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

#include "backoff.h"

class aclnt;
class xhinfo;

typedef callback<void, clnt_stat>::ref aclnt_cb;
typedef callback<void, clnt_stat, const char *, ssize_t>::ref aclntraw_cb;
typedef callback<void, ptr<aclnt>, clnt_stat>::ref aclntalloc_cb;
typedef callback<ptr<axprt_stream>, int>::ref axprtalloc_fn;
extern aclnt_cb aclnt_cb_null;
extern u_int32_t (*next_xid) ();


class callbase {
  friend class aclnt;

  virtual bool checksrc (const sockaddr *) const;
  virtual clnt_stat decodemsg (const char *, size_t) = 0;
  void expire () { tmo = NULL; finish (RPC_TIMEDOUT); }

protected:
  const ref<aclnt> c;
  const sockaddr *const dest;
  timecb_t *tmo;

  virtual ~callbase ();
  callbase (ref<aclnt>, u_int32_t, const sockaddr *);

public:
  const u_int32_t xid;
  list_entry<callbase> clink;	// Per-client list of calls
  ihash_entry<callbase> hlink;	// Link in XID hash table

  void timeout (time_t duration);
  void cancel () { delete this; }
  virtual void finish (clnt_stat) = 0;
};

class rpccb : public callbase {
protected:
  rpccb (ref<aclnt>, u_int32_t, aclnt_cb, void *, xdrproc_t, const sockaddr *);
  virtual ~rpccb () {}

  u_int32_t getxid (xdrsuio &x);
  u_int32_t getxid (char *, size_t);

public:
  aclnt_cb cb;
  void *outmem;
  xdrproc_t outxdr;

  rpccb (ref<aclnt>, xdrsuio &, aclnt_cb, void *, xdrproc_t,
	 const sockaddr *, bool doxmit = true);
  clnt_stat decodemsg (const char *, size_t);
  void finish (clnt_stat);
};

class rpccb_msgbuf : public rpccb {
protected:
  rpccb_msgbuf (ref<aclnt> c, xdrsuio &x, aclnt_cb cb,
		void *out, xdrproc_t outproc, const sockaddr *d)
    : rpccb (c, x, cb, out, outproc, d, false) {
    msglen = x.uio ()->resid ();
    msgbuf = suio_flatten (x.uio ());
  }
  rpccb_msgbuf (ref<aclnt> c, char *buf, size_t len, aclnt_cb cb,
		void *out, xdrproc_t outproc, const sockaddr *d)
    : rpccb (c, getxid (buf, len), cb, out, outproc, d),
      msgbuf (buf), msglen (len) {}
  ~rpccb_msgbuf () { xfree (msgbuf); }

public:
  void *msgbuf;
  size_t msglen;
  void xmit (int retry = 0);
};

class rpccb_unreliable : public rpccb_msgbuf {
  rpccb_unreliable ();

public:
  tmoq_entry<rpccb_unreliable> tlink;

  rpccb_unreliable (ref<aclnt>, xdrsuio &, aclnt_cb,
		    void *out, xdrproc_t outproc, const sockaddr *);
  rpccb_unreliable (ref<aclnt>, char *, size_t, aclnt_cb,
		    void *out, xdrproc_t outproc, const sockaddr *);
  ~rpccb_unreliable ();

  void timeout () { finish (RPC_TIMEDOUT); }
};

template<class T> callbase *
callbase_alloc (ref<aclnt> c, xdrsuio &x, aclnt_cb cb,
			       void *out, xdrproc_t outproc, sockaddr *d)
{
  return New T (c, x, cb, out, outproc, d);
}

class aclnt : public virtual refcount {
  friend class callbase;
  list<callbase, &callbase::clink> calls;

public:
  const ref<xhinfo> xi;
  const rpc_program &rp;

private:
  cbv::ptr eofcb;
  sockaddr *dest;

  aclnt (const axprt &);
  const aclnt &operator= (const aclnt &);

  static void seteof (ref<xhinfo>);

protected:
  aclnt (const ref<xhinfo> &x, const rpc_program &rp);
  ~aclnt ();

public:
  list_entry<aclnt> xhlink;
  const ref<axprt> &xprt () const;
  typedef callbase *(*rpccb_alloc_t) (ref<aclnt>, xdrsuio &, aclnt_cb,
				      void *, xdrproc_t, sockaddr *);
  rpccb_alloc_t rpccb_alloc;

  static void dispatch (ref<xhinfo>, const char *, ssize_t, const sockaddr *);
  static bool marshal_call (xdrsuio &, AUTH *auth, u_int32_t progno,
			    u_int32_t versno, u_int32_t procno,
			    xdrproc_t inproc, const void *in);
  bool init_call (xdrsuio &x,
		  u_int32_t procno, const void *in, void *out, aclnt_cb &,
		  AUTH *auth = NULL,
		  xdrproc_t inproc = NULL, xdrproc_t outproc = NULL,
		  u_int32_t progno = 0, u_int32_t versno = 0);

  callbase *call (u_int32_t procno, const void *in, void *out, aclnt_cb,
		  AUTH *auth = NULL,
		  xdrproc_t inproc = NULL, xdrproc_t outproc = NULL,
		  u_int32_t progno = 0, u_int32_t versno = 0,
		  sockaddr *d = NULL);
  callbase *timedcall (time_t duration,
		       u_int32_t procno, const void *in, void *out, aclnt_cb,
		       AUTH *auth = NULL,
		       xdrproc_t inproc = NULL, xdrproc_t outproc = NULL,
		       u_int32_t progno = 0, u_int32_t versno = 0,
		       sockaddr *d = NULL);
  clnt_stat scall (u_int32_t procno, const void *in, void *out,
		   AUTH *auth = NULL,
		   xdrproc_t inproc = NULL, xdrproc_t outproc = NULL,
		   u_int32_t progno = 0, u_int32_t versno = 0,
		   sockaddr *d = NULL, time_t duration = 0);

  callbase *rawcall (const char *msg, size_t len,
		     aclntraw_cb, sockaddr *dest);

  void seteofcb (cbv::ptr);

  static ptr<aclnt> alloc (ref<axprt> x, const rpc_program &pr,
			   const sockaddr *d = NULL,
			   rpccb_alloc_t ra = NULL);
};

void aclntudp_create (const char *host, int port, const rpc_program &rp,
		      aclntalloc_cb cb);
void aclntudp_create (const in_addr &addr, int port, const rpc_program &rp,
		      aclntalloc_cb cb);

extern const axprtalloc_fn axprt_stream_alloc_default;
void aclnttcp_create (const char *host, int port, const rpc_program &rp,
		      aclntalloc_cb cb,
		      axprtalloc_fn xa = axprt_stream_alloc_default);
void aclnttcp_create (const in_addr &addr, int port, const rpc_program &rp,
		      axprtalloc_fn xa = axprt_stream_alloc_default);

inline const strbuf &
strbuf_cat (const strbuf &sb, clnt_stat stat)
{
  return strbuf_cat (sb, clnt_sperrno (stat));
}
