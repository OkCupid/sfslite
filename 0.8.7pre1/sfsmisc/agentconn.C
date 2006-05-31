/* $Id$ */

/*
 *
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

#include "agentconn.h"
#include "rxx.h"

str
agentconn::lookup (str &file)
{
  str ret;

  str host = "localhost";
  int err;
  if (clnt_stat stat = cagent_ctl ()->scall (AGENTCTL_FORWARD, &host, &err)) {
    warn << "RPC error: " << stat << "\n";
    return ret;
  }
  if (err) {
    warn << "agent (ctl): " << strerror (err) << "\n";
    return ret;
  }

  sfsagent_lookup_res res;
  if (clnt_stat stat = cagent_cb ()->scall (AGENTCB_LOOKUP, &file, &res)) {
    warn << "agent (cb): " << stat << "\n";
    return ret;
  }
  if (res.type == LOOKUP_MAKELINK)
    return *res.path;
  return ret;
}

ptr<sfsagent_auth_res>
agentconn::auth (sfsagent_authinit_arg &arg)
{
  str host = "localhost";
  int err;
  ref<sfsagent_auth_res> res = New refcounted<sfsagent_auth_res> (FALSE);
  sfsagent_auth_res r;

  if (clnt_stat stat = cagent_ctl ()->scall (AGENTCTL_FORWARD, &host, &err)) {
    warn << "RPC error: " << stat << "\n";
    return res;
  }
  if (err) {
    warn << "agent (ctl): " << strerror (err) << "\n";
    return res;
  }

  if (clnt_stat stat = cagent_cb ()->scall (AGENTCB_AUTHINIT, &arg, &r)) {
    warn << "agent (cb): " << stat << "\n";
    return res;
  }
  else
    *res = r;
  return res;
}

ptr<sfsagent_rex_res>
agentconn::rex (str &pathname, bool forwardagent)
{
  ref<sfsagent_rex_res> res = New refcounted<sfsagent_rex_res> (FALSE);
  sfsagent_rex_arg a;
  a.dest = pathname;
  a.forwardagent = forwardagent;
  sfsagent_rex_res r;

  if (clnt_stat stat = cagent_ctl ()->scall (AGENTCTL_REX, &a, &r)) {
    warn << "agent (cb): " << stat << "\n";
    return NULL;
  }
  else
    *res = r;
  return res;
}

bool
agentconn::isagentrunning ()
{
  int32_t res;
  return !(ccd ()->scall (AGENT_GETAGENT, NULL, &res) 
	   || res
	   || sfscdxprt->recvfd () < 0);
}

ptr<aclnt>
agentconn::ccd (bool required)
{
  if (sfscdclnt)
    return sfscdclnt;
  int fd = required ? suidgetfd_required ("agent") : suidgetfd ("agent");
  if (fd >= 0) {
    sfscdxprt = axprt_unix::alloc (fd);
    sfscdclnt = aclnt::alloc (sfscdxprt, agent_prog_1);
  }
  return sfscdclnt;
}

int
agentconn::cagent_fd ()
{
  if (agentfd >= 0)
    return agentfd;

  static rxx sockfdre ("^-(\\d+)?$");
  if (agentsock && sockfdre.search (agentsock)) {
    if (sockfdre[1])
      agentfd = atoi (sockfdre[1]);
    else
      agentfd = 0;
    if (!isunixsocket (agentfd))
      fatal << "fd specified with '-S' not unix domain socket\n";
  }
  else if (agentsock) {
    agentfd = unixsocket_connect (agentsock);
    if (agentfd < 0)
      fatal ("%s: %m\n", agentsock.cstr ());
  }
  else {
    int32_t res;
    if (clnt_stat err = ccd ()->scall (AGENT_GETAGENT, NULL, &res))
      fatal << "sfscd: " << err << "\n";
    if (res)
      fatal << "connecting to agent via sfscd: " << strerror (res) << "\n";
    if ((agentfd = sfscdxprt->recvfd ()) < 0)
      fatal << "connecting to agent via sfscd: could not get file descriptor\n";
  }
  return agentfd;
}

ref<aclnt>
agentconn::cagent_ctl ()
{
  if (agentclnt_ctl)
    return agentclnt_ctl;

  int fd = cagent_fd ();

  agentclnt_ctl = aclnt::alloc (axprt_stream::alloc (fd), agentctl_prog_1);
  return agentclnt_ctl;
}

ref<aclnt>
agentconn::cagent_cb ()
{
  if (agentclnt_cb)
    return agentclnt_cb;

  int fd = cagent_fd ();

  agentclnt_cb = aclnt::alloc (axprt_stream::alloc (fd), agentcb_prog_1);
  return agentclnt_cb;
}
