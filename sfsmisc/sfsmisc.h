// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 1999 David Mazieres (dm@uun.org)
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

#ifndef _SFSMISC_H_
#define _SFSMISC_H_ 1

#include "amisc.h"

struct svccb;
struct aclnt;
struct asrv;
struct rabin_priv;
struct axprt_crypt;
struct sfspriv;

/* sfsconst.C */
extern u_int32_t sfs_release;
extern u_int16_t sfs_defport;
extern uid_t sfs_uid;
extern gid_t sfs_gid;
extern uid_t nobody_uid;
extern gid_t nobody_gid;
extern u_int32_t sfs_resvgid_start;
extern u_int sfs_resvgid_count;
#ifdef MAINTAINER
extern bool runinplace;
#else /* !MAINTAINER */
enum { runinplace = false };
#endif /* !MAINTAINER */
extern const char *sfsroot;
extern str sfsdir;
extern str sfssockdir;
extern str sfsdevdb;
extern const char *etc1dir;
extern const char *etc2dir;
extern const char *etc3dir;
extern const char *sfs_authd_syslog_priority;
extern u_int sfs_rsasize;
extern u_int sfs_dlogsize;
extern u_int sfs_mindlogsize;
extern u_int sfs_maxdlogsize;
extern u_int sfs_minrsasize;
extern u_int sfs_maxrsasize;
extern u_int sfs_pwdcost;
const u_int sfs_maxpwdcost = 32;
extern u_int sfs_hashcost;
extern u_int sfs_maxhashcost;

void sfsconst_init ();
str sfsconst_etcfile (const char *name);
str sfsconst_etcfile_required (const char *name);
void mksfsdir (str path, mode_t mode,
	       struct stat *sbp = NULL, uid_t uid = sfs_uid);
str sfshostname ();

#include "keyfunc.h"

template<> struct hashfn<vec<str> > {
  hashfn () {}
  hash_t operator() (const vec<str> &v) const {
    u_int val = HASHSEED;
    for (const str *sp = v.base (); sp < v.lim (); sp++)
      val = hash_bytes (sp->cstr (), sp->len (), val);
    return val;
  }
};
template<> struct equals<vec<str> > {
  equals () {}
  bool operator() (const vec<str> &a, const vec<str> &b) const {
    size_t n = a.size ();
    if (n != b.size ())
      return false;
    while (n-- > 0)
      if (a[n] != b[n])
	return false;
    return true;
  }
};

/* pathexpand.C */
int path2sch (str path, str *sch);

#endif /* _SFSMISC_H_ */
