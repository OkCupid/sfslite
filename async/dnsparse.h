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


#ifndef _DNSPARSE_H_
#define _DNSPARSE_H_ 1

#include "dns.h"

struct resrec {
  struct rd_mx {
    u_int16_t mx_pref;
    char mx_exch[MAXDNAME];
  };
  struct rd_soa {
    char soa_mname[MAXDNAME];
    char soa_rname[MAXDNAME];
    u_int32_t soa_serial;
    u_int32_t soa_refresh;
    u_int32_t soa_retry;
    u_int32_t soa_expire;
    u_int32_t soa_minimum;
  };

  char rr_name[MAXDNAME];
  u_int16_t rr_class;
  u_int16_t rr_type;
  u_int32_t rr_ttl;
  u_int16_t rr_rdlen;
  union {
    char rr_ns[MAXDNAME];
    in_addr rr_a;
    char rr_cname[MAXDNAME];
    rd_soa rr_soa;
    char rr_ptr[MAXDNAME];
    rd_mx rr_mx;
    char rr_txt[MAXDNAME];
  };
};

struct question {
  char q_name[MAXDNAME];
  u_int16_t q_class;
  u_int16_t q_type;
};

class dnsparse {
  dnsparse ();

  const u_char *const buf;
  const u_char *const eom;
  const u_char *anp;

  static int mxrec_cmp (const void *, const void *);

public:
  int error;
  const HEADER *const hdr;
  const u_int ancount;

  dnsparse (const u_char *buf, size_t len, bool answer = true);

  const u_char *getqp () { return hdr ? buf + sizeof (HEADER) : NULL; }
  const u_char *getanp () { return anp; }

  bool qparse (question *);
  bool qparse (const u_char **, question *);
  bool rrparse (const u_char **, resrec *);

  ptr<hostent> tohostent ();
  ptr<mxlist> tomxlist ();
};

#endif /* !_DNSPARSE_H_ */
