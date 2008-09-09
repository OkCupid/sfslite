
#include "rtftp.h"

bool
check_file (const rtftp_file_t &f)
{
  char buf[RTFTP_HASHSZ];
  sha1_hash (buf, f.data.base (), f.data.size ());
  return (memcmp (buf, f.hash.base (), RTFTP_HASHSZ) == 0);
}
