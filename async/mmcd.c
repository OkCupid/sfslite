/* $Id */

/*
 *
 * Copyright (C) 2004 Maxwell Krohn (max@okcupid.com)
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


#include <sys/time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "sysconf.h"

#define CLCKD_INTERVAL 10000

const char *progname;

static void
usage (int argc, char *argv[])
{
  fprintf (stderr, "usage: %s clockfile\n", argv[0]);
  exit (1);
}

static int 
timespec_diff (struct timespec a, struct timespec b)
{
  return  (a.tv_nsec - b.tv_nsec) / 1000 -
    (b.tv_sec - a.tv_sec) * 1000000;
}

static void 
mmcd_shutdown (void *rgn, size_t sz, int fd, char *fn)
{
  fprintf (stderr, 
	   "%s: caught EOF on standard input; shutting down.\n",
	   progname);
  munmap (rgn, sz);
  close (fd);

  //
  // XXX - this might segfault other processes ; not sure
  //
  unlink (fn);
}

void
setprogname (const char *in)
{
  const char *slashp = NULL;
  const char *cp;

  for (cp = in; *cp; cp++) 
    if (*cp == '/') 
      slashp = cp;

  progname = slashp ? slashp + 1 : in;
}

#define BUFSZ 1024
 
int
main (int argc, char *argv[])
{
  char *mmap_rgn;
  int fd;
  struct timespec ts[2];
  struct timespec *targ;
  struct timeval wt;
  int d;
  int rc, ctlfd, nselfd, rrc;
  fd_set read_fds;
  size_t mmap_rgn_sz;
  char buf[BUFSZ];
  int data_read;

  setprogname (argv[0]);

  //
  // listen for an EOF on Standard Input; this means it's time to
  // shutdown
  //
  ctlfd = 0;
  FD_SET (ctlfd, &read_fds);
  nselfd = ctlfd + 1;

  //
  // make the file the right size by writing our time
  // there
  //
  clock_gettime (CLOCK_REALTIME, ts);
  ts[1]  = ts[0];

  if (argc != 2) 
    usage (argc, argv);
  fd = open (argv[1], O_RDWR|O_CREAT, 0644);
  if (fd < 0) {
    fprintf (stderr, "mmcd: %s: cannot open file for reading\n", argv[1]);
    exit (1);
  }
  if (write (fd, (char *)ts, sizeof (ts)) != sizeof (ts)) {
    fprintf (stderr, "mmcd: %s: short write\n", argv[1]);
    exit (1);
  }

  mmap_rgn_sz = sizeof (struct timespec )  * 2;
  mmap_rgn = mmap (NULL, mmap_rgn_sz, PROT_WRITE, MAP_SHARED, fd, 0); 
  if (mmap_rgn == MAP_FAILED) {
    fprintf (stderr, "mmcd: mmap failed: %d\n", errno);
    exit (1);
  }

  fprintf (stderr, "%s: starting up; file=%s; pid=%d\n", 
	   progname, argv[1], getpid ());

  targ = (struct timespec *) mmap_rgn;
  wt.tv_sec = 0;
  wt.tv_usec = CLCKD_INTERVAL;
  while (1) {
    clock_gettime (CLOCK_REALTIME, ts); 
    targ[0] = ts[0];
    targ[1] = ts[0];
      
    rc = select (nselfd, &read_fds, NULL, NULL, &wt);
    if (rc < 0) {
      fprintf (stderr, "%s: select failed: %d\n", progname, errno);
      continue;
    } 

    if (rc > 0 && FD_ISSET (ctlfd, &read_fds)) {

      data_read = 0;
      while ((rrc = read (ctlfd, buf, BUFSZ)) > 0) {
	if (rrc > 0) data_read = 1;
      }

      if (rrc == 0 && !data_read) {
	mmcd_shutdown (mmap_rgn, mmap_rgn_sz, fd, argv[1]);
	break;
      }
      else if (rrc < 0) 
	fprintf (stderr, "%s: read returned error: %d\n", progname, errno);

      continue;
    }

    clock_gettime (CLOCK_REALTIME, ts+1); 
    d = timespec_diff (ts[1], ts[0]);
    
    /*
     * long sleeps will hurt the accuracy of the clock; may as well
     * report them, although eventually it would be nice to do something
     * about them on the client side;
     *
     */
    if (d > 10 * CLCKD_INTERVAL)
      fprintf (stderr, "%s: %s: very long sleep: %d\n", progname, argv[0], d);
  }
  return 0;
}
