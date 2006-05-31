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

#include "sfskeymisc.h"
#include "sfsauth_prot.h"
#include "rxx.h"

static int seqno = 1;


struct srpcon {
  const sfs_connect_cb cb;
  str user;
  str host;
  str pwd;
  ptr<sfscon> sc;
  ptr<aclnt> c;
  srp_client *srpp;
  str *pwdp;
  str *userp;
  int ntries;
  u_int32_t authno;
  int authdvers;
  bool noprompt;

  // for version 1
  sfsauth_srpres sres;

  // for version 2
  sfs_loginarg srp_larg;
  sfs_autharg2 srp_aarg; 
  sfs_loginres slres;

  srpcon (const sfs_connect_cb &c)
    : cb (c), srpp (NULL), pwdp (NULL), userp (NULL), ntries (0), 
      noprompt (false)
  { srp_aarg.set_type (SFS_SRPAUTH); }

  void fail (str msg) { (*cb) (NULL, msg); delete this; }
  void succeed () { (*cb) (sc, NULL); delete this; }
  void success ();

  void start (str &u);
  void getcon (ptr<sfscon> sc, str err);
  void v2probecb (clnt_stat err) ;
  void initsrp_v1 ();
  void initsrp_v2 ();
  void srpcb_v1 (clnt_stat err);
  void srpcb_v2 (clnt_stat err);
  bool setpwd ();
};

void
srpcon::start (str &u)
{
  static rxx usrhost ("^([^@]+)?@(.*)$");
  if (!usrhost.search (u)) {
    *userp = u;
    fail ("not of form [user]@hostname");
    return;
  }

  user = usrhost[1];
  host = usrhost[2];
  if (!user && !(user = myusername ())) {
    fail ("Could not get local username");
    return;
  }

  random_start ();

  sfs_connect_host (host, SFS_AUTHSERV, wrap (this, &srpcon::getcon), true);
}

void
srpcon::getcon (ptr<sfscon> s, str err)
{
  sc = s;
  if (!s) {
    fail (err);
    return;
  }
  if ((authdvers = sc->servinfo->get_vers ()) == 2)
    initsrp_v2 ();
  else
    initsrp_v1 ();
}

void
srpcon::initsrp_v1 ()
{
  c = aclnt::alloc (sc->x, sfsauth_program_1);
  sfssrp_init_arg arg;
  arg.username = user;
  if (!srpp->init (&arg.msg, sc->authid, user)) {
    fail ("SRP client initialization failed");
    return;
  }
  c->call (SFSAUTHPROC_SRP_INIT, &arg, &sres, wrap (this, &srpcon::srpcb_v1));
}

void
srpcon::initsrp_v2 ()
{
  srp_aarg.srpauth->req.user = user;
  srp_aarg.srpauth->req.seqno = seqno;
  srp_aarg.srpauth->req.authid = sc->authid;
  srp_aarg.srpauth->req.type = SFS_SIGNED_AUTHREQ;
  srp_larg.seqno = seqno;

  seqno++;
  
  if (!srpp->init (&(srp_aarg.srpauth->msg), sc->authid, user)) {
    fail ("SRP client initialization failed");
    return;
  }
  
  if (!xdr2bytes (srp_larg.certificate, srp_aarg)) {
    fail ("Cannot convert sfs_autharg2 to bytes via xdr2bytes");
    return;
  }

  c = aclnt::alloc (sc->x, sfs_program_1);
  c->call (SFSPROC_LOGIN, &srp_larg, &slres, wrap (this, &srpcon::srpcb_v2));
}

bool
srpcon::setpwd ()
{
  if (pwdp && *pwdp && pwdp->len ()) {
    pwd = *pwdp;
    noprompt = true;
  } else {
    pwd = getpwd (strbuf () << "Passphrase for " << srpp->getname () << ": ");
  }
  srpp->setpwd (pwd);
  if (!pwd.len ()) {
    fail ("Aborted.");
    return false;
  }
  return true;
}

void
srpcon::srpcb_v1 (clnt_stat err)
{
  if (err) {
    fail (host << ": " << err);
    return;
  }
  if (sres.status != SFSAUTH_OK) {
    if (!pwd || ntries++ >= 3 || noprompt) {
      fail ("Server aborted SRP: perhaps no user record found");
      return;
    }
    pwd = NULL;
    warnx ("Server rejected passphrase.\n");
    initsrp_v1 ();
    return;
  }

 reswitch:
  switch (srpp->next (sres.msg, sres.msg.addr ())) {
  case SRP_SETPWD:
    if (!setpwd ()) return;
    goto reswitch;
  case SRP_NEXT:
    c->call (SFSAUTHPROC_SRP_MORE, sres.msg.addr (), &sres, 
	      wrap (this, &srpcon::srpcb_v1));
    break;
  case SRP_DONE:
    success ();
    break;
  default:
    fail (host << ": server returned invalid SRP message");
    break;
  }
}

void
srpcon::srpcb_v2 (clnt_stat err)
{
  if (err) {
    fail (host << ": " << err);
    return;
  }
  if (slres.status == SFSLOGIN_BAD) {
    if (!pwd || ntries++ >= 3 || noprompt) {
      fail ("Server aborted SRP: perhaps no user record found");
      return;
    }
    pwd = NULL;
    warnx ("Server rejected passphrase.\n");
    initsrp_v2 ();
    return;
  } else if (slres.status == SFSLOGIN_OK) { 
    authno = *slres.authno;
    success ();
    return;
  }

  srpmsg msg;
  // slres.status == SFSLOGIN_MORE || SFSLOGIN_OK
 reswitch:
  switch (srpp->next (&msg, &(*slres.resmore))) {
  case SRP_SETPWD:
    if (!setpwd ()) return;
    goto reswitch;
  case SRP_NEXT:
    srp_aarg.srpauth->msg = msg;
    if (!xdr2bytes (srp_larg.certificate, srp_aarg)) {
      fail ("Cannot convert sfs_autharg2 to bytes via xdr2bytes");
      return;
    }
    c->call (SFSPROC_LOGIN, &srp_larg, &slres, wrap (this, &srpcon::srpcb_v2));
    break;
  case SRP_DONE:
    success ();
    break;
  default:
    fail (host << ": server returned invalid SRP message");
    break;
  }
}

void
srpcon::success ()
{
  if (userp)
    *userp = user << "@" << srpp->host;
  if (pwdp)
    *pwdp = pwd;
  sc->user = user;
  if (authno) { sc->auth = authuint_create (authno); }
  sc->hostid_valid = (srpp->host == sc->servinfo->get_hostname ());
  succeed ();
}
  

void
sfs_connect_srp (str &user, srp_client *srpp, sfs_connect_cb cb,
		 str *userp, str *pwdp)
{
  assert (srpp);
  srpcon *sc;
  sc = New srpcon (cb);
  sc->srpp = srpp;
  sc->pwdp = pwdp;
  sc->userp = userp;
  sc->start (user);
}

bool
get_srp_params (ptr<aclnt> c, bigint *Np, bigint *gp)
{
  bool valid = false;
  bigint N,g;
  str srpfile, parms;
  if ((srpfile = sfsconst_etcfile ("sfs_srp_parms")) &&
      (parms = file2str (srpfile)) &&
      import_srp_params (parms, &N, &g) ) {
    if (!srp_base::checkparam (N, g)) {
      warn << "Invalid SRP parameters read from file: "
	   << srpfile << "\n";
    } else {
      valid = true;
    }
  }

  if (!valid && c) {
    sfsauth2_query_arg aqa;
    sfsauth2_query_res aqr;
    aqa.type = SFSAUTH_SRPPARMS;
    aqa.key.set_type (SFSAUTH_DBKEY_NULL);
    clnt_stat err = c->scall (SFSAUTH2_QUERY, &aqa, &aqr);
    if (!err && aqr.type == SFSAUTH_SRPPARMS &&
	import_srp_params (aqr.srpparms->parms, &N, &g))
      if (!srp_base::checkparam (N, g)) {
	warn << "Invalid SRP parameters read from sfsauthd.\n";
	return false;
      } else {
	valid = true;
      }
  }
  if (valid) {
    *Np = N;
    *gp = g;
    return true;
  }
  return false;
}
