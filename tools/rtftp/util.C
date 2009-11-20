
#include "rtftp.h"
#include "rtftp_prot.h"
#include "sha1.h"
#include "serial.h"

//-----------------------------------------------------------------------

bool
check_file (const rtftp_file_t &f)
{
  char buf[RTFTP_HASHSZ];
  sha1_hash (buf, f.data.base (), f.data.size ());
  return (memcmp (buf, f.hash.base (), RTFTP_HASHSZ) == 0);
}

//-----------------------------------------------------------------------

static int
dir_ok (const char *d)
{
  int rc;
  struct stat sb;
  if (stat (d, &sb) == 0) {
    if (S_ISDIR (sb.st_mode)) rc = 1;
    else rc = -1;
  } else {
    rc = 0;
  }
  return rc;
}

//-----------------------------------------------------------------------

static const char *
mywd ()
{
#define SZ 1024
  static char buf[SZ];
  return getcwd (buf, SZ);
#undef SZ
}

//-----------------------------------------------------------------------

static int 
mymkdir (char *dir, char *end)
{
  assert (*end == '/');
  assert (dir < end);
  *end = 0;
  int rc = dir_ok (dir);
  if (rc < 0) {
    warn ("cannot access dir %s from dir %s\n", dir, mywd ());
  } else if (rc == 1) {
    rc = 0;
  } else if ((rc = mkdir (dir, 0777)) < 0) {
    warn ("mkdir failed in %s on dir %s: %m\n", mywd (), dir);
  }
  *end = '/';
  return rc;
}

//-----------------------------------------------------------------------

static int
mkdir_p (char *s)
{
  int rc = 0;
  for (char *p = s; rc == 0 && p && *p; ) {
    p = strchr (p, '/');
    if (p && *p) {
      rc = mymkdir (s, p);
      p++;
    }
  }
  if (rc != 0) {
    warn ("cannot create file %s due to mkdir failures\n", s);
  } else if (!s || !*s) {
    warn ("no file to write!\n");
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------

static bool
my_str2file (str fn, str dat, int mode, bool do_fsync)
{
  str tmp = strbuf ("%s.%d~", fn.cstr (), rand ());
  const char *tmpc = tmp.cstr ();
  int fd = ::open (tmpc, O_WRONLY | O_CREAT, mode);
  int rc = 0;
  
  if (fd < 0) {
    warn ("failed to open file %s: %m\n", tmpc);
  } else {
    size_t bsz = 8196;
    const char *bp = dat.cstr ();
    const char *endp = dat.cstr () + dat.len ();
    
    while (bp < endp && rc >=0 ) {
      size_t len = min<size_t> (bsz, endp - bp);
      rc = write (fd, bp, len);
      if (rc < 0) {
	warn ("write failed on file %s: %m\n", tmpc);
      } else if (rc == 0) {
	warn ("unexpected 0-length write on file %s\n", tmpc);
	rc = EINVAL;
      } else {
	bp += rc;
	rc = 0;
      }
    }
    close (fd);
  }
  if (do_fsync && !fsync (fd)) {
    warn ("fsync failed on file %s: %m\n", tmpc);
  }
  if (rc != 0) {
    unlink (tmpc);
  } else if ((rc = rename (tmpc, fn.cstr ())) != 0)  {
    warn ("failed to rename %s to %s: %m\n", tmpc, fn.cstr ());
  }
  return (rc == 0);
}

//-----------------------------------------------------------------------

//
// write out the data 's' to the file given by _file.  Might need to make
// some parent directories if the filename contains slashes.
//
// return -1 if failure, and 0 if wrote the file out OK.
//
int write_file (const str &nm, const str &dat, bool do_fsync)
{   
  char *s = strdup (nm.cstr ());
  int rc = 0;

  rc = mkdir_p (s);

  if (rc == 0 && !my_str2file (nm, dat, 0666, do_fsync)) {
    warn ("cannot create file %s: %m\n", nm.cstr ());
    rc = -1;
  } else {
    rc = 0;
  }

  if (s) free (s);
  return rc;
}

//-----------------------------------------------------------------------

int
open_file (const str &nm, int flags)
{
  char *s = strdup (nm.cstr ());
  int rc = 0;

  rc = mkdir_p (s);
  if (rc == 0)
    rc = ::open (nm.cstr (), flags, 0666);

  if (s) free (s);
  return rc;
}

//-----------------------------------------------------------------------
