/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 * Copyright (C) 1999 Michael Kaminsky (kaminsky@lcs.mit.edu)
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

#include "agentmisc.h"
#include "agentconn.h"
#include "crypt.h"
#include "rxx.h"

str dotsfs; 
str agentsock;
str userkeysdir;

void
agent_setsock ()
{
  if (!dotsfs) {
    const char *home = getenv ("HOME");
    if (!home)
      fatal ("No HOME environment variable set\n");
    dotsfs = strbuf ("%s/.sfs", home);
  }
  if (!userkeysdir) 
    userkeysdir = strbuf ("%s/authkeys", dotsfs.cstr ());
      
  if (!agentsock)
    if (char *p = getenv ("SFS_AGENTSOCK"))
      agentsock = p;
}

void
agent_ckdir (bool fail_on_keysdir)
{
  if (!agentsock)
    agent_setsock ();
  if (!agent_ckdir (dotsfs))
    exit (1);
  if (!agent_ckdir (userkeysdir) && fail_on_keysdir)
    exit (1);
}

bool
agent_ckdir (const str &s)
{
  assert (s);
  struct stat sb;
  if (stat (s, &sb) < 0) {
    warn ("%s: %m\n", s.cstr ());
    return false;
  }
  if (!S_ISDIR (sb.st_mode)) {
    warn << s << ": not a directory\n";
    return false;
  }
  if (sb.st_mode & 077) {
    warn ("%s: mode 0%o should be 0700\n", s.cstr (),
	  int (sb.st_mode & 0777));
    return false;
  }
  return true;
}

void
agent_mkdir (const str &dir)
{
  if (!dir)
    return;
  struct stat sb;
  if (stat (dir, &sb) < 0) {
    if (errno != ENOENT)
      fatal ("%s: %m\n", dir.cstr ());
    warn << "creating directory " << dir << "\n";
    if (mkdir (dir, 0700) < 0 || stat (dir, &sb) < 0)
      fatal ("%s: %m\n", dir.cstr ());
  }
  if (!S_ISDIR (sb.st_mode))
    fatal << dir << ": not a directory\n";
  if (sb.st_mode & 077)
    fatal ("%s: mode 0%o should be 0700\n", dir.cstr (),
	   int (sb.st_mode & 0777));
}


void
agent_mkdir ()
{
  if (!agentsock)
    agent_setsock ();
  agent_mkdir (dotsfs);
  agent_mkdir (userkeysdir);
}

str
defkey ()
{
  agent_ckdir ();
  return dotsfs << "/identity";
}

void
rndaskcd ()
{
  static bool done;
  if (done)
    return;
  done = true;

  sfsagent_seed seed;
  ref<agentconn> aconn = New refcounted<agentconn> ();
  ptr<aclnt> c = aconn->ccd (false);
  if (!c || c->scall (AGENT_RNDSEED, NULL, &seed))
    warn ("sfscd not running, limiting sources of entropy\n");
  else
    rnd_input.update (seed.base (), seed.size ());
}

bool
isagentrunning ()
{
  ref <agentconn> aconn = New refcounted<agentconn> ();
  return aconn->isagentrunning ();
}

void
agent_spawn (bool opt_verbose)
{
  // start agent if needed (sfsagent -c)
  if (!isagentrunning ()) {
    if (opt_verbose)
      warn << "No existing agent found; spawning a new one...\n";
    str sa = find_program ("sfsagent");
    vec<char *> av;

    av.push_back (const_cast<char *> (sa.cstr ()));
    av.push_back ("-c");
    av.push_back (NULL);
    pid_t pid = spawn (av[0], av.base ());
    if (waitpid (pid, NULL, 0) < 0)
      fatal << "Could not spawn a new SFS agent: " <<
	strerror (errno) << "\n";
  }
  else {
    if (opt_verbose)
      warn << "Contacting existing agent...\n";
  }
}
