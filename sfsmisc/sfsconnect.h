/* $Id$ */

/*
 *
 * Copyright (C) 1999 David Mazieres (dm@uun.org)
 * Copyright (C) 2000 Michael Kaminsky (kaminsky@lcs.mit.edu)
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

#ifndef _SFSCONNECT_H_
#define _SFSCONNECT_H_ 1

#include "async.h"
#include "arpc.h"
#include "crypt.h"
#include "sfsmisc.h"
#include "sfsauth_prot.h"

struct sfscon {
  ptr<axprt_crypt> x;
  str path;
  ptr<const sfs_servinfo_w> servinfo;
  sfs_authinfo authinfo;
  sfs_hash hostid;
  sfs_hash sessid;
  sfs_hash authid;
  bool hostid_valid;
  AUTH *auth;
  str user;
  sfscon () : hostid_valid (false), auth (NULL) {}
  ~sfscon () { if (auth) auth_destroy (auth); }
};

void sfs_initci (sfs_connectinfo *ci, str path, sfs_service service,
		 vec<sfs_extension> *exts = NULL);
bool sfs_nextci (sfs_connectinfo *ci);
typedef callback<void, ptr<sfscon>, str>::ref sfs_connect_cb;
typedef callback<void, str>::ref sfs_dologin_cb;
void sfs_connect (const sfs_connectarg &carg, sfs_connect_cb cb,
		  bool encrypt = true, bool check_hostid = true);
void sfs_connect_path (str path, sfs_service service, sfs_connect_cb cb,
		       bool encrypt = true, bool check_hostid = true);
void sfs_connect_host (str host, sfs_service service, sfs_connect_cb cb,
		       bool encrypt = true);
void sfs_connect_withx (const sfs_connectarg &carg, sfs_connect_cb cb,
                        ptr<axprt> xx, bool encrypt = true,
			bool check_hostid = true);

void sfs_dologin (ptr<sfscon> sc, sfspriv *k, int seqno, sfs_dologin_cb cb,
		  bool cred = true);
void sfs_login_signed (ptr<sfscon> scon, sfs_dologin_cb cb,
		       ptr<sfs_autharg2> aarg, str err,
		       ptr<sfs_sig2> sig);
void sfs_login_done (ptr<sfscon> scon, ptr<sfs_loginres> lres,
		     sfs_dologin_cb cb, clnt_stat st);
#endif /* _SFSCONNECT_H_ */
