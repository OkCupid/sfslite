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

/* DNS packet parsing.  This code is easy to understand if and only if
 * you also have a copy of RFC 1035.  You can get this by anonymous
 * ftp from, among other places, ftp.internic.net:/rfc/rfc1035.txt.
 */

#include "dnsparse.h"
#include "arena.h"
#include "vec.h"
#include "qhash.h"

extern "C" {
#ifdef NEED_DN_SKIPNAME_DECL
int dn_skipname (const u_char *, const u_char *);
#endif /* NEED_DN_SKIPNAME_DECL */
#ifdef NEED_DN_EXPAND_DECL
int dn_expand (const u_char *, const u_char *, const u_char *, char *, int);
#endif /* NEED_DN_EXPAND_DECL */
}

#if 0
int
dn_skipname (const u_char *icp, const u_char *eom)
{
  const u_char *cp = icp;
  int n;

  while (cp < eom && (n = *cp++))
    switch (n & 0xc0) {
    case 0:
      cp += n;
      break;
    case 0xc0:
      cp++;
      break;
    default:
      return -1;
    }
  return cp > eom ? -1 : cp - icp;
}
#endif

dnsparse::dnsparse (const u_char *buf, size_t len, bool answer)
  : buf (buf), eom (buf + len),
    anp (NULL), error (0),
    hdr (len > sizeof (HEADER) ? (HEADER *) buf : NULL),
    ancount (hdr ? ntohs (hdr->ancount) : 0),
    nscount (hdr ? ntohs (hdr->nscount) : 0),
    arcount (hdr ? ntohs (hdr->arcount) : 0)
{
  if (!hdr)
    error = ARERR_BADRESP;
  else if (hdr->rcode)
    error = hdr->rcode;
  else if ((!hdr->qr && answer) || (hdr->qr && !answer))
    error = ARERR_BADRESP;
  else if (!ntohs (hdr->qdcount))
    error = ARERR_BADRESP;
#if 0
  else if (hdr->tc)
    error = ARERR_BADRESP;
#endif
  else {
    const u_char *cp = getqp ();
    for (int i = 0, l = ntohs (hdr->qdcount); i < l; i++) {
      int n = dn_skipname (cp, eom);
      cp += n + 4;
      if (n < 0 || cp > eom) {
	error = ARERR_BADRESP;
	return;
      }
    }
    anp = cp;
  }
}

bool
dnsparse::qparse (const u_char **cpp, question *qp)
{
  const u_char *cp = *cpp;
  int n = dn_expand (buf, eom, cp, qp->q_name, sizeof (qp->q_name));
  if (n < 0 || n >= MAXDNAME || cp + n + 4 > eom)
    return false;
  cp += n;
  GETSHORT (qp->q_type, cp);
  GETSHORT (qp->q_class, cp);
  *cpp = cp;
  return true;
}

bool
dnsparse::qparse (question *qp)
{
  const u_char *cp = getqp ();
  return cp && qparse (&cp, qp);
}

bool
dnsparse::rrparse (const u_char **cpp, resrec *rrp)
{
  const u_char *cp = *cpp;
  const u_char *e;
  u_int16_t rdlen;
  int n;

  if ((n = dn_expand (buf, eom, cp, rrp->rr_name,
		      sizeof (rrp->rr_name))) < 0
      || cp + n + 10 > eom)
    return false;
  cp += n;
  GETSHORT (rrp->rr_type, cp);
  GETSHORT (rrp->rr_class, cp);
  GETLONG (rrp->rr_ttl, cp);
  GETSHORT (rdlen, cp);
  rrp->rr_rdlen = rdlen;
  if ((e = cp + rdlen) > eom)
    return false;
  switch (rrp->rr_type) {
  case T_A:
    memcpy (&rrp->rr_a, cp, sizeof (rrp->rr_a));
    cp += sizeof (rrp->rr_a);
    break;
  case T_NS:
  case T_CNAME:
  case T_PTR:
    if ((n = dn_expand (buf, eom, cp, rrp->rr_cname,
			sizeof (rrp->rr_cname))) < 0)
      return false;
    cp += n;
    break;
  case T_MX:
  case T_AFSDB:
    if (rdlen < 2)
      return false;
    GETSHORT (rrp->rr_mx.mx_pref, cp);
    if ((n = dn_expand (buf, eom, cp, rrp->rr_mx.mx_exch,
			sizeof (rrp->rr_mx.mx_exch))) < 0)
      return false;
    cp += n;
    break;
  case T_TXT:
    if (rdlen >= sizeof (rrp->rr_txt) || rdlen < 1
	|| rdlen < *cp + 1)
      return false;
    memcpy (rrp->rr_txt, cp + 1, *cp);
    rrp->rr_txt[rdlen] = '\0';
    cp += rdlen;
    break;
  case T_SOA:
    if ((n = dn_expand (buf, eom, cp, rrp->rr_soa.soa_mname,
			sizeof (rrp->rr_soa.soa_mname))) < 0)
      return false;
    cp += n;
    if ((n = dn_expand (buf, eom, cp, rrp->rr_soa.soa_rname,
			sizeof (rrp->rr_soa.soa_rname))) < 0)
      return false;
    cp += n;
    if (cp + 20 > eom)
      return false;
    GETLONG (rrp->rr_soa.soa_serial, cp);
    GETLONG (rrp->rr_soa.soa_refresh, cp);
    GETLONG (rrp->rr_soa.soa_retry, cp);
    GETLONG (rrp->rr_soa.soa_expire, cp);
    GETLONG (rrp->rr_soa.soa_minimum, cp);
    break;
  case T_SRV:
    if (rdlen < 7)
      return false;
    GETSHORT (rrp->rr_srv.srv_prio, cp);
    GETSHORT (rrp->rr_srv.srv_weight, cp);
    GETSHORT (rrp->rr_srv.srv_port, cp);
    if ((n = dn_expand (buf, eom, cp, rrp->rr_srv.srv_target,
			sizeof (rrp->rr_srv.srv_target))) < 0)
      return false;
    cp += n;
    break;
  default:
    cp = e;
    break;
  }
  assert (cp == e);
  *cpp = cp;
  return true;
}

/* Takes the response from an DNS query, and turns it into a
 * dynamically allocated hostent structure (defined in netdb.h).  All
 * data for the hostent structure is allocated contiguously, so that
 * it can all be freed by simply freeing the address of the hostent
 * structure returned.  If no addresses are returned (i.e. no A
 * records returned by the query), then a valid hostent will still be
 * returned, but with h->h_addr_list[0] will be NULL.
 */
ptr<hostent>
dnsparse::tohostent ()
{
  const u_char *cp = getanp ();
  arena a;
  vec<in_addr> av;
  char *name = NULL;
  char *cname = NULL;

  if (!cp)
    return NULL;

  for (size_t i = 0; i < ancount; i++) {
    resrec rr;
    if (!rrparse (&cp, &rr)) {
      error = ARERR_BADRESP;
      return NULL;
    }
    if (rr.rr_class == C_IN)
      switch (rr.rr_type) {
      case T_A:
	if (!name)
	  name = a.strdup (rr.rr_name);
	av.push_back (rr.rr_a);
	break;
      case T_CNAME:
	if (!cname)
	  cname = a.strdup (rr.rr_name);
	break;
      }
  }
  if (!name) {
    error = ARERR_NXREC;
    return NULL;
  }

  ref<hostent> h = refcounted<hostent, vsize>::alloc
    (sizeof (*h)
     + (!!cname + 2 + av.size ()) * sizeof (char *)
     + av.size () * sizeof (in_addr)
     + strlen (name) + 1
     + (cname ? strlen (cname) + 1 : 0));
  h->h_addrtype = AF_INET;
  h->h_length = sizeof (in_addr);
  h->h_aliases = (char **) &h[1];
  h->h_addr_list = &h->h_aliases[1+!!cname];

  size_t i;
  for (i = 0; i < av.size (); i++) {
    h->h_addr_list[i] = ((char *) &h->h_addr_list[1 + av.size ()]
			 + i * sizeof (in_addr));
    *(in_addr *) h->h_addr_list[i] = av[i];
  }
  h->h_addr_list[i] = NULL;
  h->h_name = ((char *) &h->h_addr_list[1 + av.size ()]
	       + i * sizeof (in_addr));
  strcpy (h->h_name, name);
  if (cname) {
    h->h_aliases[0] = h->h_name + strlen (h->h_name) + 1;
    h->h_aliases[1] = NULL;
    strcpy (h->h_aliases[0], cname);
  }
  else
    h->h_aliases[0] = NULL;

  return h;
}

int
dnsparse::mxrec_cmp (const void *_a, const void *_b)
{
  const mxrec *a = (mxrec *) _a, *b = (mxrec *) _b;
  int d = (int) a->pref - (int) b->pref;
  if (d)
    return d;
  return strcasecmp (a->name, b->name);
}

ptr<mxlist>
dnsparse::tomxlist ()
{
  const u_char *cp = getanp ();
  arena a;
  char *name = NULL;
  int n_chars = 0;

  if (!cp)
    return NULL;

  u_int nmx = 0;
  mxrec *mxes = (mxrec *) a.alloc ((1 + ancount) * sizeof (*mxes));

  for (u_int i = 0; i < ancount; i++) {
    resrec rr;
    if (!rrparse (&cp, &rr)) {
      error = ARERR_BADRESP;
      return NULL;
    }
    if (rr.rr_class == C_IN && rr.rr_type == T_MX) {
      u_int16_t pr = rr.rr_mx.mx_pref;
      u_int mxi;

      if (!name)
	name = a.strdup (rr.rr_name);
      else if (strcasecmp (name, rr.rr_name))
	continue;

      for (mxi = 0; mxi < nmx && strcasecmp (rr.rr_mx.mx_exch,
					     mxes[mxi].name); mxi++)
	;
      if (mxi < nmx) {
	if (pr < mxes[mxi].pref)
	  mxes[mxi].pref = pr;
      }
      else {
	n_chars += strlen (rr.rr_mx.mx_exch) + 1;
	mxes[nmx].pref = pr;
	mxes[nmx].name = a.strdup (rr.rr_mx.mx_exch);
	nmx++;
      }
    }
  }
  if (nmx == 0) {
    error = ARERR_NXREC;
    return NULL;
  }
  else if (nmx > 1)
    qsort (mxes, nmx, sizeof (*mxes), mxrec_cmp);

  n_chars += strlen (name) + 1;

  ref <mxlist> mxl = refcounted<mxlist, vsize>::alloc
    (offsetof (mxlist, m_mxes[nmx]) + n_chars);
  mxrec *mxrecs = mxl->m_mxes;
  char *np = (char *) &mxrecs[nmx];

  mxl->m_nmx = nmx;
  mxl->m_name = np;
  strcpy (np, name);
  np += strlen (np) + 1;

  for (u_int i = 0; i < nmx; i++) {
    mxrecs[i].pref = mxes[i].pref;
    mxrecs[i].name = np;
    strcpy (np, mxes[i].name);
    np += strlen (np) + 1;
  }

  return mxl;
}

int
dnsparse::srvrec_cmp (const void *_a, const void *_b)
{
  const srvrec *a = (srvrec *) _a, *b = (srvrec *) _b;
  if (int d = (int) a->prio - (int) b->prio)
    return d;
  return (int) b->weight - (int) a->weight;
}

void
dnsparse::srvrec_randomize (srvrec *base, srvrec *last)
{
  qsort (base, last - base, sizeof (*base), &srvrec_cmp);

#if 0
  for (srvrec *x = base; x < last; x++)
    warn ("DBG:  %3d %3d %5d %s\n", x->prio, x->weight, x->port, x->name);
#endif

  while (base < last) {
    srvrec *lastprio = base;
    u_int totweight;

    /* It's not clear from RFC 2782 if we need to randomize multiple
     * records with weight 0 at the same priority.  We do it anyway
     * just because it might be helpful. */
    if (base->weight == 0) {
      totweight = 1;
      while (++lastprio < last && lastprio->prio == base->prio)
	totweight++;
      while (base + 1 < lastprio) {
	u_int which = arandom () % totweight;
	if (which) {
	  srvrec tmp = base[which];
	  base[which] = *base;
	  *base = tmp;
	}
	base++;
	totweight--;
      }
    }
    else {
      totweight = lastprio->weight;
      while (++lastprio < last && lastprio->weight
	     && lastprio->prio == base->prio)
	totweight += lastprio->weight;

      while (base + 1 < lastprio) {
	u_int32_t rndweight = arandom () % totweight + 1;
	srvrec *nextrec;
	for (nextrec = base; nextrec->weight < rndweight; nextrec++)
	  rndweight -= nextrec->weight;
	srvrec tmp = *base;
	*base = *nextrec;
	*nextrec = tmp;
	totweight -= base++->weight;
      }
      assert (totweight == base->weight);
    }

    base++;
  }
}

ptr<srvlist>
dnsparse::tosrvlist ()
{
  const u_char *cp = getanp ();
  arena a;
  int n_chars = 0;
  char *name = NULL;
  srvrec *recs;

  if (!cp)
    return NULL;

  u_int nsrvs = 0;
  recs = (srvrec *) a.alloc ((1 + ancount) * sizeof (*recs));

  for (u_int i = 0; i < ancount; i++) {
    resrec rr;
    if (!rrparse (&cp, &rr)) {
      error = ARERR_BADRESP;
      return NULL;
    }
    if (rr.rr_class != C_IN || rr.rr_type != T_SRV)
      continue;
    if (!name) {
      name = a.strdup (rr.rr_name);
      n_chars += strlen (name) + 1;
    }
    else if (strcasecmp (name, rr.rr_name))
      continue;

#if 1
    for (u_int i = 0; i < nsrvs; i++)
      if (recs[i].prio == rr.rr_srv.srv_prio
	  && recs[i].port == rr.rr_srv.srv_port
	  && !strcasecmp (recs[i].name, rr.rr_srv.srv_target)) {
	rr.rr_srv.srv_weight += recs[i].weight;
	continue;
      }
#endif

    srvrec &sr = recs[nsrvs++];
    sr.prio = rr.rr_srv.srv_prio;
    sr.weight = rr.rr_srv.srv_weight;
    sr.port = rr.rr_srv.srv_port;
    sr.name = a.strdup (rr.rr_srv.srv_target);
    n_chars += strlen (sr.name) + 1;
  }

  srvrec_randomize (recs, recs + nsrvs);

  ref<srvlist> s = refcounted<srvlist, vsize>::alloc
    (offsetof (srvlist, s_srvs[nsrvs]) + n_chars);
  s->s_nsrv = nsrvs;
  char *np = reinterpret_cast<char *> (&s->s_srvs[nsrvs]);
  s->s_name = np;
  strcpy (np, name);
  np += strlen (np) + 1;

  for (u_int i = 0; i < s->s_nsrv; i++) {
    s->s_srvs[i].prio = recs[i].prio;
    s->s_srvs[i].weight = recs[i].weight;
    s->s_srvs[i].port = recs[i].port;
    s->s_srvs[i].name = np;
    strcpy (np, recs[i].name);
    np += strlen (np) + 1;
  }
  
  return s;
}


#if 1
void
printaddrs (const char *msg, ptr<hostent> h, int dns_errno)
{
  char **aliases;
  struct in_addr **addrs;

  if (msg)
    printf ("%s (hostent):\n", msg);

  if (! h) {
    printf ("   Error: %s\n", dns_strerror (dns_errno));
    return;
  }

  aliases = h->h_aliases;
  addrs = (struct in_addr **) h->h_addr_list;

  printf ("    Name: %s\n", h->h_name);
  while (*aliases)
    printf ("   Alias: %s\n", *aliases++);
  while (*addrs)
    printf (" Address: %s\n", inet_ntoa (**addrs++));
}

void
printmxlist (const char *msg, ptr<mxlist> m, int dns_errno)
{
  int i;

  if (msg)
    printf ("%s (mxlist):\n", msg);

  if (!m) {
    printf ("   Error: %s\n", dns_strerror (dns_errno));
    return;
  }

  printf ("    Name: %s\n", m->m_name);
  for (i = 0; i < m->m_nmx; i++)
    printf ("      MX: %3d %s\n", m->m_mxes[i].pref, m->m_mxes[i].name);
}

void
printsrvlist (const char *msg, ptr<srvlist> s, int dns_errno)
{
  int i;

  if (msg)
    printf ("%s (srvlist):\n", msg);

  if (!s) {
    printf ("   Error: %s\n", dns_strerror (dns_errno));
    return;
  }

  printf ("    Name: %s\n", s->s_name);
  for (i = 0; i < s->s_nsrv; i++)
    printf ("     SRV: %3d %3d %3d %s\n", s->s_srvs[i].prio,
	    s->s_srvs[i].weight, s->s_srvs[i].port, s->s_srvs[i].name);
}
#endif
