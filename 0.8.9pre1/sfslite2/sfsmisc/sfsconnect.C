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

#include "sfsconnect.h"
#include "rxx.h"
#include "parseopt.h"
#include "sfscrypt.h"

void
sfs_initci (sfs_connectinfo *ci, str path, sfs_service service,
	    vec<sfs_extension> *exts)
{
  ci->set_civers (5);
  ci->ci5->release = sfs_release;
  ci->ci5->service = service;
  ci->ci5->sname = path;
  if (exts)
    ci->ci5->extensions.set (exts->base (), exts->size ());
  else
    ci->ci5->extensions.setsize (0);
}

bool
sfs_nextci (sfs_connectinfo *ci)
{
  if (ci->civers == 5) {
    sfs_service service = ci->ci5->service;
    str sname = ci->ci5->sname;
    rpc_vec<sfs_extension, RPC_INFINITY> exts;
    exts.swap (ci->ci5->extensions);

    ci->set_civers (4);
    ci->ci4->service = service;
    if (!sfs_parsepath (sname, &ci->ci4->name, &ci->ci4->hostid))
      ci->ci4->name = sname;
    exts.swap (ci->ci4->extensions);
    return true;
  }
  return false;
}

struct constate {
  static ptr<sfspriv> ckey;

  const sfs_connect_cb cb;

  str sname;
  u_int16_t port;
  str location;

  sfs_connectarg carg;
  sfs_connectres cres;

  ref<sfscon> sc;
  ptr<aclnt> c;
  
  bool encrypt;
  bool check_hostid;
  bool has_x;
  
  constate (const sfs_connect_cb &c)
    : cb (c), port (SFS_PORT), sc (New refcounted<sfscon>),
      encrypt (true), check_hostid (true), has_x (false) {}

  void fail (str msg) { (*cb) (NULL, msg); delete this; }
  void succeed () { c = NULL; (*cb) (sc, NULL); delete this; }

  void start ();
  void getfd (int fd);
  void sendconnect ();
  void getconres (enum clnt_stat err);
  void cryptcb (const sfs_hash *sessidp);
};

ptr<sfspriv> constate::ckey;

void
constate::start ()
{
  /* XXX */
//   if (encrypt)
//     rndstart ();
  if (sname == "-" || (location && location == "-")) {
    assert(!has_x);
    location = "localhost";
    int fd = suidgetfd ("authd");
    errno = ECONNREFUSED;
    getfd (fd);
  }
  else if (!location && !sfs_parsepath (sname, &location, NULL, &port))
    fail (sname << ": cannot parse path");
  else if (!strchr (location, '.') && check_hostid)
    fail (location << ": must contain fully qualified domain name");
  else if (!has_x)
    tcpconnect (location, port, wrap (this, &constate::getfd));
  else
    sendconnect();
}

void
constate::getfd (int fd)
{
  if (fd < 0) {
    fail (location << ": Could not connect to host: " << strerror (errno));
    return;
  }
  if (!isunixsocket (fd))
    tcp_nodelay (fd);
  sc->x = axprt_crypt::alloc (fd);
  c = aclnt::alloc (sc->x, sfs_program_1);
  sendconnect ();
}

void
constate::sendconnect ()
{
  c->call (SFSPROC_CONNECT, &carg, &cres, wrap (this, &constate::getconres));
}

void
constate::getconres (enum clnt_stat err)
{
  if (err == RPC_CANTDECODEARGS && sfs_nextci (&carg)) {
    sendconnect ();
    return;
  }
  if (err) {
    fail (location << ": " << err);
    return;
  }
  if (cres.status) {
    fail (location << ": " << cres.status);
    return;
  }
  ref<const sfs_servinfo_w> si = sfs_servinfo_w::alloc (cres.reply->servinfo);
  sc->servinfo = si;
  if (!si->mkhostid_client (&sc->hostid)) {
    fail (location << ": Server returned garbage hostinfo");
    return;
  }

  if (si->ckpath (sname))
    sc->hostid_valid = true;
  else if (check_hostid) {
    fail (location << ": Server key does not match hostid");
    return;
  }
  sc->path = si->mkpath (2, port); 

  if (encrypt) {
    if (!ckey) {
      ckey = sfscrypt.gen (SFS_RABIN, 0, SFS_DECRYPT);
    }
    sfs_client_crypt (c, ckey, carg, *cres.reply, si,
		      wrap (this, &constate::cryptcb));
  }
  else
    succeed ();
}

void
constate::cryptcb (const sfs_hash *sidp)
{
  if (!sidp) {
    fail (location << ": Session key negotiation failed");
    return;
  }
  sc->sessid = *sidp;
  sfs_get_authid (&sc->authid,
		  carg.civers == 4 ? carg.ci4->service : carg.ci5->service,
		  sc->servinfo->get_hostname (), &sc->hostid, 
		  &sc->sessid, &sc->authinfo);
  succeed ();
}

void
sfs_connect (const sfs_connectarg &carg, sfs_connect_cb cb,
	     bool encrypt, bool check_hostid)
{
  constate *sc = New constate (cb);
  assert (carg.civers == 5);
  sc->carg = carg;
  sc->sname = carg.ci5->sname;
  sc->encrypt = encrypt;
  sc->check_hostid = check_hostid;
  sc->start ();
}

void
sfs_connect_path (str path, sfs_service service, sfs_connect_cb cb,
		  bool encrypt, bool check_hostid)
{
  constate *cs = New constate (cb);
  cs->sname = path;
  sfs_initci (&cs->carg, path, service);
  cs->encrypt = encrypt;
  cs->check_hostid = check_hostid;
  cs->start ();
}

void
sfs_connect_local_cb (constate *cs, ptr<sfscon> sc, str err)
{
  if (sc) {
    cs->sname = sc->path;
    cs->check_hostid = true;
    cs->start ();
  }
  else
    cs->fail (err);
}

void
sfs_connect_host (str host, sfs_service service, sfs_connect_cb cb,
		  bool encrypt)
{
  static rxx portrx ("^(.*)%(\\d+)$");
  constate *cs = New constate (cb);
  int prt;
  if (portrx.match (host) && convertint (portrx[2], &prt)) {
    cs->port = prt;
    cs->sname = cs->location = portrx[1];
  }
  else
    cs->sname = cs->location = host;
  sfs_initci (&cs->carg, host, service);
  cs->encrypt = encrypt;
  cs->check_hostid = false;
  if (host == "-") {
    sfs_connectarg carg;
    sfs_initci (&carg, host, SFS_AUTHSERV);
    sfs_connect (carg, wrap (sfs_connect_local_cb, cs), false, false);
    return;
  }
  cs->start ();
}

void
sfs_connect_withx (const sfs_connectarg &carg, sfs_connect_cb cb,
                   ptr<axprt> xx, bool encrypt, bool check_hostid)
{
  constate *cs = New constate (cb);
  assert (carg.civers == 5);
  cs->carg = carg;
  cs->sname = carg.ci5->sname;
  cs->encrypt = encrypt;
  cs->check_hostid = check_hostid;
  cs->has_x = true;
  cs->sc->x = NULL;
  cs->c = aclnt::alloc (xx, sfs_program_1);
  cs->start ();
}

void
sfs_dologin (ptr<sfscon> scon, sfspriv *key, int seqno, sfs_dologin_cb cb,
	     bool cred)
{
  assert (scon);
  ref<aclnt> c (aclnt::alloc (scon->x, sfsauth_prog_2));
  ptr<sfs_autharg2> aarg = New refcounted<sfs_autharg2> (SFS_AUTHREQ2);
  aarg->sigauth->req.type = cred ? SFS_SIGNED_AUTHREQ : 
    SFS_SIGNED_AUTHREQ_NOCRED;
  aarg->sigauth->req.authid = scon->authid;
  aarg->sigauth->req.seqno = seqno;
  if (!key->export_pubkey (&aarg->sigauth->key)) {
    (*cb) ("Login failed: could not export public key");
    return;
  }
  sfsauth2_sigreq sr (SFS_SIGNED_AUTHREQ);
  *sr.authreq = aarg->sigauth->req;
  key->sign (sr, scon->authinfo, 
	     wrap (&sfs_login_signed, scon, cb, aarg));
}

void
sfs_login_signed (ptr<sfscon> scon, sfs_dologin_cb cb, ptr<sfs_autharg2> aarg,
		  str err, ptr<sfs_sig2> sig)
{
  if (!sig) {
    (*cb) (err);
    return;
  }
  ref<aclnt> c (aclnt::alloc (scon->x, sfs_program_1));
  sfs_loginarg larg;
  aarg->sigauth->sig = *sig;
  
  if (!xdr2bytes (larg.certificate, *aarg)) {
    (*cb) ("Login failed: could not marshall login certificate");
    return;
  }
  larg.seqno = 0;
  ptr<sfs_loginres> lres = New refcounted<sfs_loginres> ();
  c->call (SFSPROC_LOGIN, &larg, lres, 
	   wrap (&sfs_login_done, scon, lres, cb));
}

void
sfs_login_done (ptr<sfscon> scon, ptr<sfs_loginres> lres, sfs_dologin_cb cb,
		clnt_stat st)
{
  if (st) {
    strbuf b;
    b << st;
    (*cb) (b);
    return ;
  }

  if (lres->status != SFSLOGIN_OK || !(*lres->authno)) {
    (*cb) ("Login / authentication refused by auth server");
    return ;
  }
  scon->auth = authuint_create (*lres->authno);
  (*cb) (NULL);
  return;
}

