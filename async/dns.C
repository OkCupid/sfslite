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


/* Asynchronous resolver.  This code is easy to understand if and only
 * if you also have a copy of RFC 1035.  You can get this by anonymous
 * ftp from, among other places, ftp.internic.net:/rfc/rfc1035.txt.
 */

#include "dnsparse.h"
#include "arena.h"

#include "vec.h"
#include "ihash.h"
#include "list.h"
#include "backoff.h"

INITFN(dns_start);

#define NSPORT NAMESERVER_PORT
#define QBSIZE 512

class dnsreq {
  static u_int16_t lastid;	// Last request id
  static int ns_idx;
  static sockaddr_in srvaddr;	// IP address of name server
  static int sock;		// UDP socket

  static bool tcplock;
  static bool reloadlock;
  static int last_good_idx;
  static time_t last_resp;
  static time_t last_reload;

  static void tcpcb (int, int);
  static void resend (dnsreq *r) { r->xmit (); }
  static u_int16_t genid ();

  dnsreq ();
  virtual void readreply (dnsparse *) = 0;

  void fail (int);
  void start (bool);

  int ntries;
  int srchno;

protected:
  int error;

public:
  static void reloadcb (bool force_setsock, str newres);
  static void setsock (bool);
  static void pktready ();

  u_int16_t id;			// DNS query ID
  str basename;			// Name for which to search
  str name;			// Name for which to query
  u_int16_t type;		// Type of query (T_A, T_PTR, etc.)

  ihash_entry<dnsreq> hlink;	// Per-id hash table link
  tmoq_entry<dnsreq> tlink;	// Retransmit queue link

  dnsreq (str, u_int16_t, bool search = false);
  virtual ~dnsreq ();

  bool xmit (int = 0);
  void timeout ();
};

u_int16_t dnsreq::lastid;
int dnsreq::ns_idx;
sockaddr_in dnsreq::srvaddr;
int dnsreq::sock = -1;
bool dnsreq::tcplock;
bool dnsreq::reloadlock;
int dnsreq::last_good_idx;
time_t dnsreq::last_resp;
time_t dnsreq::last_reload;


static ihash<u_int16_t, dnsreq, &dnsreq::id, &dnsreq::hlink> reqtab;
static tmoq<dnsreq, &dnsreq::tlink, 1, 5> reqtoq;

static void dns_reload_dumpres (int fd);

u_int16_t
dnsreq::genid ()
{
  return arandom () & 0xffff;
}

void
dnsreq::fail (int err)
{
  if (!error)
    error = err;
  readreply (NULL);
}

void
dnsreq::start (bool again)
{
  if (again && (srchno < 0 || !_res.dnsrch[srchno])) {
    fail (NXDOMAIN);
    return;
  }

  if (again) {
    reqtab.remove (this);
    reqtoq.remove (this);
  }
  if (srchno >= 0)
    name = strbuf ("%s.%s", basename.cstr (), _res.dnsrch[srchno++]);
  id = genid ();
  reqtab.insert (this);
  reqtoq.start (this);
}

dnsreq::dnsreq (str n, u_int16_t t, bool search)
  : ntries (0), error (0), type (t)
{
  if (!search || strchr (n, '.')) {
    srchno = -1;
    basename = NULL;
    name = n;
  }
  else {
    srchno = 0;
    basename = n;
    name = NULL;
  }
  start (false);
}

dnsreq::~dnsreq ()
{
  reqtab.remove (this);
  reqtoq.remove (this);
}

//* Send a query to the nameserver */
bool
dnsreq::xmit (int retry)
{
  u_char qb[QBSIZE];
  HEADER *const h = (HEADER *) qb;
  int n;

  n = res_mkquery (QUERY, name, C_IN, type, NULL, 0, NULL, qb, sizeof (qb));
  if (n < 0)
    return false;
  h->id = htons (id);  /* XXX - Why bother with htons? */
  if (sendto (sock, reinterpret_cast<char *> (qb), n, 0,
	      reinterpret_cast<sockaddr *> (&srvaddr), sizeof (srvaddr)) < 0) {
    error = ARERR_CANTSEND;
    return false;
  }
  error = 0;
  return true;
}

void
dnsreq::timeout ()
{
  if (timenow - last_resp > 60) {
    if (++ntries < _res.nscount) {
      reqtab.remove (this);
      reqtoq.remove (this);
      setsock (true);
      start (false);
      return;
    }
    setsock (true);
  }
  fail (ARERR_TIMEOUT);
}

void
dnsreq::pktready ()
{
  u_char qb[QBSIZE];
  ssize_t n;

  for (;;) {
    sockaddr_in sin;
    socklen_t sinlen = sizeof (sin);
    bzero (&sin, sizeof (sin));
    n = recvfrom (sock, reinterpret_cast<char *> (qb), sizeof (qb),
		  0, reinterpret_cast<sockaddr *> (&sin), &sinlen);
    if (n < 0)
      break;
    if (!addreq (reinterpret_cast<sockaddr *> (&sin),
		 reinterpret_cast<sockaddr *> (&srvaddr),
		 sizeof (sin)))
      continue;
   
    last_resp = timenow;
    last_good_idx = ns_idx;

    dnsparse reply (qb, n);
    question q;
    if (!reply.qparse (&q))
      continue;

    dnsreq *r;
    for (r = reqtab[ntohs (reply.hdr->id)];
	 r && (r->type != q.q_type || strcasecmp (r->name, q.q_name));
	 r = reqtab.nextkeq (r))
      ;
    if (!r)
      continue;

    if (reply.error && !r->error)
      r->error = reply.error;
    if (r->error == NXDOMAIN) {
      r->error = 0;
      r->start (true);
    }
    else {
      r->readreply (r->error ? NULL : &reply);
      //delete r;
    }
  }
}

/* Try setting up a TCP connection to name servers, to make sure they
 * are really running named.  (This is really a kludge because we
 * can't see ICMP port unreachable packets without using connected UDP
 * sockets, and we can't use connected UDP sockets without failing
 * when laptops change their IP addresses.) */
void
dnsreq::tcpcb (int idx, int fd)
{
  tcplock = false;
  if (fd >= 0) {
    close (fd);
    last_good_idx = ns_idx;
    return;
  }
  else if (idx == ns_idx && !last_resp)
    setsock (true);
}

void
dnsreq::setsock (bool bump)
{
  if (sock < 0) {
#if 0
    /* We use a reserved port here to prevent port-capture problems.
     * (An unprivileged user can steal packets destined to an INADDR_ANY
     * socket by binding to the same port on the more-specific interface
     * address. */
    if (suidsafe ())
      sock = inetsocket_resvport (SOCK_DGRAM);
    else
      sock = inetsocket (SOCK_DGRAM);
#else
    /* It seems many Linux distros automatically firewall all UDP
     * traffic to reserved ports without even keeping state.  That
     * means reserved UDP ports essentially don't work.  Oh well.  */
    sock = inetsocket (SOCK_DGRAM);
#endif
    if (sock < 0)
      fatal ("dnsreq::setsock: inetsocket: %s\n", strerror (errno));
    close_on_exec (sock);
    make_async (sock);
    fdcb (sock, selread, wrap (pktready));
  }

  if (!bump)
    last_reload = time (NULL);
  else if (timenow > last_reload + 60 && !reloadlock) {
    reloadlock = true;
    chldrun (wrap (dns_reload_dumpres), wrap (&dnsreq::reloadcb, true));
    return;
  }

  if (bump && _res.nscount) {
    ns_idx = (ns_idx + 1) % _res.nscount;
    warn ("changing nameserver to %s\n",
	  inet_ntoa (_res.nsaddr_list[ns_idx].sin_addr));
  }

  srvaddr = _res.nsaddr_list[ns_idx];
  if (!srvaddr.sin_addr.s_addr)	// XXX
    srvaddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  last_resp = 0;
  reqtab.traverse (wrap (resend));

  if (!tcplock && (!bump || ns_idx != last_good_idx)) {
    tcplock = true;
    tcpconnect (srvaddr.sin_addr, ntohs (srvaddr.sin_port),
		wrap (tcpcb, ns_idx));
  }
}

void
dnsreq_cancel (dnsreq *rqp)
{
  delete rqp;
}

class dnsreq_a : public dnsreq {
  bool checkaddr;		// Non-zero when arr_addr must be checked
  in_addr addr;			// Adress of inverse queries (for checking)
  cbhent cb;			// Callback for hostbyname/addr
  dnsreq_a ();
public:
  dnsreq_a (str n, cbhent c, bool s = false)
    : dnsreq (n, T_A, s), checkaddr (false), cb (c) {}
  dnsreq_a (str n, cbhent c, const in_addr &a)
    : dnsreq (n, T_A), checkaddr (true), addr (a), cb (c) {}
  void readreply (dnsparse *);
};

void
dnsreq_a::readreply (dnsparse *reply)
{
  ptr<hostent> h;
  if (!error) {
    if (!(h = reply->tohostent ()))
      error = reply->error;
    else if (checkaddr) {
      char **ap;
      for (ap = h->h_addr_list; *ap && *(in_addr *) *ap != addr; ap++)
	;
      if (!*ap) {
	h = NULL;
	error = ARERR_PTRSPOOF;
      }
    }
  }
  (*cb) (h, error);
  delete this;
}

dnsreq *
dns_hostbyname (str name, cbhent cb,
		bool search, bool addrok)
{
  if (addrok) {
    in_addr addr;
    if (inet_aton (name.cstr (), &addr)) {
      ptr<hostent> h = refcounted<hostent, vsize>::alloc
	(sizeof (*h) + 3 * sizeof (void *)
	 + sizeof (addr) + strlen (name) + 1);
      h->h_aliases = (char **) &h[1];
      h->h_addrtype = AF_INET;
      h->h_length = sizeof (addr);
      h->h_addr_list = &h->h_aliases[1];

      h->h_aliases[0] = NULL;
      h->h_addr_list[0] = (char *) &h->h_addr_list[2];
      h->h_addr_list[1] = NULL;

      *(struct in_addr *) h->h_addr_list[0] = addr;
      h->h_name = (char *) h->h_addr_list[0] + sizeof (addr);
      strcpy ((char *) h->h_name, name);

      (*cb) (h, 0);
      return NULL;
    }
  }
  return New dnsreq_a (name, cb, search);
}

class dnsreq_mx : public dnsreq {
  cbmxlist cb;
  dnsreq_mx ();
public:
  dnsreq_mx (str n, cbmxlist c, bool s)
    : dnsreq (n, T_MX, s), cb (c) {}
  void readreply (dnsparse *);
};

void
dnsreq_mx::readreply (dnsparse *reply)
{
  ptr<mxlist> m;
  if (!error) {
    if (!(m = reply->tomxlist ()))
      error = reply->error;
  }
  (*cb) (m, error);
  delete this;
}

dnsreq *
dns_mxbyname (str name, cbmxlist cb, bool search)
{
  return New dnsreq_mx (name, cb, search);
}

class dnsreq_srv : public dnsreq {
  cbsrvlist cb;
  dnsreq_srv ();
public:
  dnsreq_srv (str n, cbsrvlist c, bool s)
    : dnsreq (n, T_SRV, s), cb (c) {}
  void readreply (dnsparse *);
};

void
dnsreq_srv::readreply (dnsparse *reply)
{
  ptr<srvlist> s;
  if (!error) {
    if (!(s = reply->tosrvlist ()))
      error = reply->error;
  }
  (*cb) (s, error);
  delete this;
}

dnsreq *
dns_srvbyname (str name, cbsrvlist cb, bool search)
{
  return New dnsreq_srv (name, cb, search);
}

class dnsreq_ptr : public dnsreq {
  in_addr addr;
  cbhent cb;			// Callback for hostbyname/addr
  dnsreq_a *vrfyreq;
public:
  static str inaddr_arpa (in_addr);
  dnsreq_ptr (in_addr a, cbhent c)
    : dnsreq (inaddr_arpa (a), T_PTR), addr (a), cb (c), vrfyreq (NULL) {}
  ~dnsreq_ptr () { delete vrfyreq; }
  void readreply (dnsparse *);
  void readvrfy (ptr<hostent> h, int err);
};

str
dnsreq_ptr::inaddr_arpa (in_addr addr)
{
  u_char *a = reinterpret_cast<u_char *> (&addr);
  return strbuf ("%d.%d.%d.%d.in-addr.arpa", a[3], a[2], a[1], a[0]);
}

void
dnsreq_ptr::readreply (dnsparse *reply)
{
  if (!error) {
    const u_char *cp = reply->getanp ();
    for (u_int i = 0; i < reply->ancount; i++) {
      resrec rr;
      if (!reply->rrparse (&cp, &rr))
	break;
      if (rr.rr_type == T_PTR && rr.rr_class == C_IN) {
	vNew dnsreq_a (rr.rr_ptr, wrap (this, &dnsreq_ptr::readvrfy), addr);
	return;
      }
    }
  }
  if (!error && !(error = reply->error))
    error = ARERR_NXREC;
  (*cb) (NULL, error);
  delete this;
}

void
dnsreq_ptr::readvrfy (ptr<hostent> h, int err)
{
  vrfyreq = NULL;
  (*cb) (h, err);
  delete this;
}

dnsreq *
dns_hostbyaddr (in_addr addr, cbhent cb)
{
  return New dnsreq_ptr (addr, cb);
}


void
dnsreq::reloadcb (bool force_setsock, str newres)
{
  reloadlock = false;
  last_reload = timenow;
  if (!newres) {
    warn ("fork: %m\n");
    if (force_setsock)
      setsock (true);
    return;
  }
  if (newres.len () != sizeof (_res)) {
    warn ("dns_reload: short read\n");
    if (force_setsock)
      setsock (true);
    return;
  }

  char oldnsaddr[sizeof (_res.nsaddr_list)];
  memcpy (oldnsaddr, _res.nsaddr_list, sizeof (oldnsaddr));
  memcpy (&_res, newres, sizeof (_res));
  if (memcmp (oldnsaddr, _res.nsaddr_list, sizeof (oldnsaddr))) {
    warn ("reloaded resolv.conf file\n");
    ns_idx = last_good_idx = _res.nscount ? _res.nscount - 1 : 0;
    setsock (true);
  }
  else if (force_setsock)
    setsock (true);
}

static void
dns_reload_dumpres (int fd)
{
  bzero (&_res, sizeof (_res));
  res_init ();
  write (fd, &_res, sizeof (_res));
  _exit (0);
}
void
dns_reload ()
{
  chldrun (wrap (dns_reload_dumpres), wrap (&dnsreq::reloadcb, false));
}

static void
dns_start ()
{
  if (!(_res.options & RES_INIT))
    res_init ();
  dnsreq::setsock (false);
}

const char *
dns_strerror (int no)
{
  switch (no) {
  case NOERROR:
    return "no error";
  case FORMERR:
    return "DNS format error";
  case SERVFAIL:
    return "name server failure";
  case NXDOMAIN:
    return "non-existent domain name";
  case NOTIMP:
    return "unimplemented DNS request";
  case REFUSED:
    return "DNS query refused";
  case ARERR_NXREC:
    return "no DNS records of appropriate type";
  case ARERR_TIMEOUT:
    return "name lookup timed out";
  case ARERR_PTRSPOOF:
    return "incorrect PTR record";
  case ARERR_BADRESP:
    return "malformed DNS reply";
  case ARERR_CANTSEND:
    return "cannot send to name server";
  case ARERR_REQINVAL:
    return "malformed domain name";
  case ARERR_CNAMELOOP:
    return "CNAME records form loop";
  default:
    return "unknown DNS error";
  }
}

int
dns_tmperr (int no)
{
  switch (no) {
  case SERVFAIL:
  case ARERR_TIMEOUT:
  case ARERR_CANTSEND:
  case ARERR_BADRESP:
    return 1;
  default:
    return 0;
  }
}

