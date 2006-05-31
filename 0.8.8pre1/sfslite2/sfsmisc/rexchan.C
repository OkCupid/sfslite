/* $Id$ */

/*
 *
 * Copyright (C) 2001-2003 Michael Kaminsky (kaminsky@lcs.mit.edu)
 * Copyright (C) 2000-2001 Eric Peterson (ericp@lcs.mit.edu)
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

#include "rex.h"

bool rexfd::garbage_bool;

#ifndef O_ACCMODE
# define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#endif /* !O_ACCMODE */

bool
is_fd_wronly (int fd)
{
  int n;
  if ((n = fcntl (fd, F_GETFL)) < 0)
    fatal ("fcntl failed on fd = %d\n", fd);
  return (n & O_ACCMODE) == O_WRONLY;
}

bool
is_fd_rdonly (int fd)
{
  int n;
  if ((n = fcntl (fd, F_GETFL)) < 0)
    fatal ("fcntl failed on fd = %d\n", fd);
  return (n & O_ACCMODE) == O_RDONLY;
}

void
unixfd::readeof ()
{
  if (!reof) {
    rex_payload payarg;
    payarg.channel = channo;
    payarg.fd = fd;
    proxy->call (REX_DATA, &payarg, &garbage_bool, aclnt_cb_null);
    update_connstate (SHUT_RD);
  }
}

void 
unixfd::rcb ()
{
  if (reof)
    return;

  char buf[16*1024];
  int fdrecved = -1;
  ssize_t n;
  if (unixsock)
    n = readfd (localfd_in, buf, sizeof (buf), &fdrecved);
  else
    n = read (localfd_in, buf, sizeof (buf));

  if (n < 0) {
    if (errno != EAGAIN)
      abort ();
    return;
  }

  if (!n) {
    readeof ();
    return;
  }
  else {
    rex_payload arg;
    arg.channel = channo;
    arg.fd = fd;
    arg.data.set (buf, n);

    if (fdrecved >= 0) {
      close_on_exec (fdrecved);
      rex_newfd_arg arg;
      arg.channel = channo;
      arg.fd = fd;
      ref<rex_newfd_res> resp (New refcounted<rex_newfd_res> (false));
      proxy->call (REX_NEWFD, &arg, resp,
		   wrap (mkref (this), &unixfd::newfdcb, fdrecved, resp));
    }

    ref<bool> pres (New refcounted<bool> (false));
    rsize += n;
    proxy->call (REX_DATA, &arg, pres,
		 wrap (mkref (this), &unixfd::datacb, n, pres));
  }

  if (rsize >= hiwat)
    fdcb (localfd_in, selread, NULL);
}

void
unixfd::newfdcb (int fdrecved, ptr<rex_newfd_res> resp, clnt_stat)
{
  if (!resp->ok) {
    close (fdrecved);
    return;
  }
  vNew refcounted<unixfd> (pch, *resp->newfd, fdrecved);
}

void
unixfd::newfd (svccb *sbp)
{
  assert (paios_out);

  rexcb_newfd_arg *argp = sbp->template getarg<rexcb_newfd_arg> ();
    
  int s[2];
    
  if(socketpair(AF_UNIX, SOCK_STREAM, 0, s)) {
    warn << "error creating socketpair";
    sbp->replyref (false);
    return;
  }
    
  make_async (s[1]);
  make_async (s[0]);
  close_on_exec (s[1]);
  close_on_exec (s[0]);
    
  paios_out->sendfd (s[1]);
    
  vNew refcounted<unixfd> (pch, argp->newfd, s[0]);
  sbp->replyref (true);
}

void
unixfd::data (svccb *sbp)
{
  assert (paios_out);
    
  rex_payload *argp = sbp->template getarg<rex_payload> ();
    
  if (argp->data.size () > 0) {
    if (weof) {
      sbp->replyref (false);
      return;
    }
    else {
      str data (argp->data.base (), argp->data.size ());
      paios_out << data;
      sbp->replyref (true);
    }
  }
  else {
    sbp->replyref (true);
      
    //we don't shutdown immediately to give data a chance to
    //asynchronously flush
    paios_out->setwcb (wrap (this, &unixfd::update_connstate, SHUT_WR));
  }
}

/* unixfd specific arguments:

   localfd_in:	local file descriptor for input (or everything for non-tty)

   localfd_out: local file descriptor for output (only used for tty support)
     Set localfd_out = -1 (or omit the argument entirely) if you're not
     working with a remote tty; then the localfd_in is connected directly
     to the remote FD for both reads and writes (except it it's RO or WO).

   noclose: Unixfd will not use close or shutdown calls on the local
     file descriptor (localfd_in); useful for terminal descriptors,
     which must hang around so that raw mode can be disabled, etc.

   shutrdonexit: When the remote module exits, we shutdown the read
     direction of the local file descriptor (_in).  This isn't always
     done since not all file descriptors managed on the REX channel
     are necessarily connected to the remote module.
*/
unixfd::unixfd (rexchannel *pch, int fd, int localfd_in, int localfd_out,
		bool noclose, bool shutrdonexit, cbv closecb)
  : rexfd::rexfd (pch, fd),
    localfd_in (localfd_in), localfd_out (localfd_out), rsize (0),
    unixsock (isunixsocket (localfd_in)), weof (false), reof (false),
    shutrdonexit (shutrdonexit), closecb (closecb)
{
  if (noclose) {
    int duplocalfd = dup (localfd_in);
    if (duplocalfd < 0)
      warn ("failed to duplicate fd for noclose behavior (%m)\n");
    else
      unixfd::localfd_in = duplocalfd;
  }

  make_async (this->localfd_in);
  if (!is_fd_wronly (this->localfd_in))
    fdcb (localfd_in, selread, wrap (this, &unixfd::rcb));
    
  /* for tty support we split the input/output to two local FDs */
  if (localfd_out >= 0)
    paios_out = aios::alloc (this->localfd_out);
  else
    paios_out = aios::alloc (this->localfd_in);
}


void
rexchannel::remove_fd (int fdn)
{
//   warn << "--reached remove_fd (" << fdn << "), fdc = " << fdc << "\n";
  vfds[fdn] = NULL;
  if (!--fdc)
    sess->remove_chan (channo); 
}

void
rexchannel::abort () {
  size_t lvfds = vfds.size ();
  for (size_t f = 0; f < lvfds; f++)
    if (vfds[f])
      vfds[f]->abort ();
}

void
rexchannel::quit ()
{
  //  warn << "--entering rexchannel::quit\n";
  rex_int_arg arg;
  arg.channel = channo;
  arg.val = 15;
  proxy->call (REX_KILL, &arg, &rexfd::garbage_bool, aclnt_cb_null);
}

void
rexchannel::channelinit (u_int32_t chnumber, ptr<aclnt> proxyaclnt, int error)
{
  proxy = proxyaclnt;
  channo = chnumber;
  madechannel (error);
}

void
rexchannel::data (svccb *sbp)
{
  assert (sbp->prog () == REXCB_PROG && sbp->proc () == REXCB_DATA);
  rex_payload *dp = sbp->template getarg<rex_payload> ();
  assert (dp->channel == channo);
  if (dp->fd < 0 ||
      implicit_cast<size_t> (dp->fd) >= vfds.size () ||
      !vfds[dp->fd]) {
    warn ("payload fd %d out of range\ndata:%s\n", dp->fd,
	  dp->data.base ());
    sbp->replyref (false);
    return;
  }

  vfds[dp->fd]->data (sbp);
}

void
rexchannel::newfd (svccb *sbp)
{
  assert (sbp->prog () == REXCB_PROG && sbp->proc () == REXCB_NEWFD);
  rexcb_newfd_arg *arg = sbp->template getarg<rexcb_newfd_arg> ();

  int fd = arg->fd;

  if (fd < 0 || implicit_cast<size_t> (fd) >= vfds.size () || !vfds[fd]) {
    warn ("newfd received on invalid fd %d at rexchannel::newfd\n", fd);
    sbp->replyref (false);
    return;
  }
      
  vfds[fd]->newfd (sbp);
}

void
rexchannel::exited (int status)
{
  for (size_t ix = 0; ix < vfds.size();  ix++) {
    if (!vfds[ix]) continue;
    vfds[ix]->exited ();
  } 
}

void
rexchannel::insert_fd (int fdn, ptr<rexfd> rfd)
{
  assert (fdn >= 0);

//   warn ("--reached insert_fd (%d)\n", fdn);
  size_t oldsize = vfds.size ();
  size_t neededsize = fdn + 1;
    
  if (neededsize > oldsize) {
    vfds.setsize (neededsize);
    for (int ix = oldsize; implicit_cast <size_t> (ix) < neededsize; ix++)
      vfds[ix] = NULL;
  }
    
  if (vfds[fdn]) {
    warn ("creating fd on busy fd %d at rexfd::rexfd, overwriting\n", fdn);
    assert (false);
  }
    
  vfds[fdn] = rfd;
  fdc++;
}

void
rexsession::rexcb_dispatch (svccb *sbp)
{
  if (!sbp) {
    warn << "rexcb_dispatch: error\n";
    if (endcb) endcb ();
    return;
  }
      
  switch (sbp->proc ()) {
	
  case REXCB_NULL:
    sbp->reply (NULL);
    break;
	
  case REXCB_EXIT:
    {
      rex_int_arg *argp = sbp->template getarg<rex_int_arg> ();
      rexchannel *chan = channels[argp->channel];
	  
      if (chan) {
	chan->got_exit_cb = true;
	chan->exited (argp->val);
      }
      else {	// the channel was already shutdown "by EOF's"
	chan = channels_pending_exit[argp->channel];
	assert (chan);
	chan->got_exit_cb = true;
	chan->exited (argp->val);
	remove_chan_pending_exit (argp->channel);
      }

      sbp->reply (NULL);
      break;
    }
	
  case REXCB_DATA:
    {
      rex_payload *argp = sbp->template getarg<rex_payload> ();
      rexchannel *chan = channels[argp->channel];

      if (chan)
	chan->data (sbp);
      else			    
	sbp->replyref (false);

      break;
    }
	
  case REXCB_NEWFD:
    {
      rex_int_arg *argp = sbp->template getarg<rex_int_arg> ();
      rexchannel *chan = channels[argp->channel];

      if (chan)
	chan->newfd (sbp);
      else
	sbp->replyref (false);

      break;
    }
    
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}
  
void
rexsession::madechannel (ptr<rex_mkchannel_res> resp, 
			 ptr<rexchannel> newchan, clnt_stat err)
{
  if (err) {
    warn << "REX_MKCHANNEL RPC failed (" << err << ")\n";
    newchan->channelinit (0, proxy, 1);
  }
  else if (resp->err != SFS_OK) {
    warn << "REX_MKCHANNEL failed (" << int (resp->err) << ")\n";
    newchan->channelinit (0, proxy, 1);
  }
  else {
    cchan++;

    if (verbose) {
      warn << "made channel: ";
      vec<str> command = newchan->get_cmd ();
      for (size_t i = 0; i < command.size (); i++)
	warnx << command[i] << " ";
      warnx << "\n";
    }
    channels.insert (resp->resok->channel, newchan);
    newchan->channelinit (resp->resok->channel, proxy, 0);
  }
}
    
void
rexsession::seq2sessinfo (u_int64_t seqno, sfs_hash *sidp, sfs_sessinfo *sip,
			  rex_sesskeydat *kcsdat, rex_sesskeydat *kscdat)
{
  kcsdat->seqno = seqno;
  kscdat->seqno = seqno;
    
  sfs_sessinfo si;
  si.type = SFS_SESSINFO;
  si.kcs.setsize (sha1::hashsize);
  sha1_hashxdr (si.kcs.base (), *kcsdat, true);
  si.ksc.setsize (sha1::hashsize);
  sha1_hashxdr (si.ksc.base (), *kscdat, true);
    
  if (sidp)
    sha1_hashxdr (sidp->base (), si, true);
  if (sip)
    *sip = si;
    
  bzero (si.kcs.base (), si.kcs.size ());
  bzero (si.ksc.base (), si.ksc.size ());
}

void
rexsession::attached (rexd_attach_res *resp, ptr<axprt_crypt> sessxprt,
		      sfs_sessinfo *sessinfo, clnt_stat err)
{
  if (err) {
    fatal << "FAILED (" << err << ")\n";
  }
  else if (*resp != SFS_OK) {
    //warn << "FAILED (attach err " << int (*resp) << ")\n";
    warn << "Unable to attach to proxy..."
	 << "killing cached connection and retrying.\n";
    delete sessinfo;
    delete resp;
    connect (true);
    return;
  }
  delete resp;
  if (verbose)
    warn << "attached\n";
    
  proxyxprt = axprt_crypt::alloc (sessxprt->reclaim ());
  proxyxprt->encrypt (sessinfo->kcs.base (), sessinfo->kcs.size (),
		      sessinfo->ksc.base (), sessinfo->ksc.size ());
    
  bzero (sessinfo->kcs.base (), sessinfo->kcs.size ());
  bzero (sessinfo->ksc.base (), sessinfo->ksc.size ());
  delete sessinfo;
    
  proxy = aclnt::alloc (proxyxprt, rex_prog_1);
  rexserv = asrv::alloc (proxyxprt, rexcb_prog_1,
			 wrap (this, &rexsession::rexcb_dispatch));
    
  sessioncreatedcb ();
}

void
rexsession::connected (rex_sesskeydat *kcsdat, rex_sesskeydat *kscdat,
		       sfs_seqno *rexseqno, ptr<sfscon> sc, str err)
{
  if (!sc) {
    fatal << schost << ": FAILED (" << err << ")\n";
  }
    
  ptr<axprt_crypt> sessxprt = sc->x;
  ptr<aclnt> sessclnt = aclnt::alloc (sessxprt, rexd_prog_1);
    
  rexd_attach_arg arg;
    
  arg.seqno = *rexseqno;
  sfs_sessinfo *sessinfo = New sfs_sessinfo;
    
  seq2sessinfo (0, &arg.sessid, NULL, kcsdat, kscdat);
  seq2sessinfo (arg.seqno, &arg.newsessid, sessinfo, kcsdat, kscdat);
    
  // ECP comment: why doesn't agent just give us sessid,newsessid,sessinfo??

  rexd_attach_res *resp = New rexd_attach_res;
  sessclnt->call (REXD_ATTACH, &arg, resp, wrap (this,
						 &rexsession::attached,
						 resp, sessxprt, sessinfo));
  delete kcsdat;
  delete kscdat;
  delete rexseqno;
}

void
rexsession::connect (bool bypass_cache)
{
  ref<agentconn> aconn = New refcounted<agentconn> ();

  if (bypass_cache) {
    sfs_hostname arg = schost;
    bool res;
    if (clnt_stat err = 
	aconn->cagent_ctl ()->scall (AGENTCTL_KILLSESS, &arg, &res))
      fatal << "agent: " << err << "\n";
    if (!res)
      fatal << "no rexsessions connected to " << schost << "\n";
  }

  ptr<sfsagent_rex_res> ares = aconn->rex (schost, forwardagent);
  if (!ares)
    fatal << "could not connect to agent\n";
  if (!ares->status)
    fatal << "agent failed to establish a connection to "
	  << schost << ".\n"
	  << "     Perhaps you don't have permissions to login there.\n"
	  << "     Perhaps you don't have any keys loaded in your agent.\n"
	  << "     Perhaps the remote machine is not running a REX server.\n";
  
  rex_sesskeydat *kscdat = New rex_sesskeydat;
  rex_sesskeydat *kcsdat = New rex_sesskeydat;
  sfs_seqno *rexseqno = New sfs_seqno;
	    
  kcsdat->type = SFS_KCS;
  kcsdat->cshare = ares->resok->kcs.kcs_share;
  kcsdat->sshare = ares->resok->kcs.ksc_share;
  kscdat->type = SFS_KSC;
  kscdat->cshare = ares->resok->ksc.kcs_share;
  kscdat->sshare = ares->resok->ksc.ksc_share;
  *rexseqno = ares->resok->seqno;
  sfs_connect_path (schost, SFS_REX,
		    wrap (this, &rexsession::connected,
			   kcsdat, kscdat, rexseqno),
		    false);
}

//use this one if you already have an encrypted transport connected to proxy
rexsession::rexsession (str schostname, ptr<axprt_crypt> proxyxprt)
  : verbose (false), proxyxprt (proxyxprt),
    cchan (0), endcb (NULL), schost (schostname)
{
  proxy = aclnt::alloc (proxyxprt, rex_prog_1);
  rexserv = asrv::alloc (proxyxprt, rexcb_prog_1,
			 wrap (this, &rexsession::rexcb_dispatch));
}

rexsession::rexsession (cbv scb, str schostname, 
			bool fa)
  : verbose (false), 
    cchan (0), endcb (NULL),
    schost (schostname), sessioncreatedcb (scb), forwardagent (fa)
{
  connect ();
}

void
rexsession::makechannel (ptr<rexchannel> newchan, rex_env env)
{
  rex_mkchannel_arg arg;

  vec<str> command = newchan->get_cmd ();
  arg.av.setsize (command.size ());
  for (size_t i = 0; i < command.size (); i++)
    arg.av[i] = command[i];
  arg.nfds = newchan->get_initnfds ();
  arg.env = env;
    
  ref<rex_mkchannel_res> resp (New refcounted<rex_mkchannel_res> ());
  proxy->call (REX_MKCHANNEL, &arg, resp, wrap (this,
						&rexsession::madechannel,
						resp, newchan));
}

void
rexsession::remove_chan (int channo)
{
  /* warn << "--reached remove_chan; cchan = " << cchan << "\n"; */
  if (channels[channo] && !channels[channo]->got_exit_cb) {
    /* warn << "--remove_chan: removing chan but haven't seen REXCB_EXIT yet\n"; */
    channels_pending_exit.insert (channo, channels[channo]);
    channels.remove (channo);
    return;
  }
  channels.remove (channo);
  if (!--cchan) {
    /* warn << "--remove_chan: removing last channel\n"; */
    if (endcb)
      endcb ();
  }
}

void
rexsession::remove_chan_pending_exit (int channo)
{
  /* warn << "--reached remove_chan_pending_exit; cchan = " << cchan << "\n"; */
  channels_pending_exit.remove (channo);
  if (!--cchan) {
    /* warn << "--remove_chan_pending_exit: removing last channel\n"; */
    if (endcb)
      endcb ();
  }
}

