
#include "rtftp.h"
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

//
// write out the data 's' to the file given by _file.  Might need to make
// some parent directories if the filename contains slashes.
//
// return -1 if failure, and 0 if wrote the file out OK.
//
int write_file (const str &nm, const str &dat)
{   
  char *s = strdup (nm.cstr ());
  char *file = NULL;
  char *np;
  int rc = 0;
  for (char *p = s; rc >= 0 && p && *p; np = p) {
    char *sl = strchr (p, '/');
    if (sl && *sl) {
      file = np = sl ++;
      *np = 0;
      int rc = dir_ok (p);
      if (rc < 0) {
	warn ("cannot access dir %s from dir %s\n", p, mywd ());
      } else if (mkdir (p, 0777) != 0) {
	warn ("mkdir failed in %s on dir %s: %m\n", mywd (), p);
	rc = -1;
      }
      if (rc == 0 && chdir (p) != 0) {
	warn ("cannot chdir to %s from dir %s: %m\n", p, mywd ());
	rc = -1;
      }
    }
  }
  if (!file || *file) {
    warn ("no file to write!\n");
    rc = -1;
  } else if (rc == 0 && !str2file (file, dat, 0666, false)) {
    warn ("cannot create file %s in dir %s: %m\n", file, mywd ());
    rc = -1;
  } else {
    rc = 0;
  }
  if (s) free (s);
  return rc;
}
