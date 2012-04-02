
#include "rxx.h"
#include "str.h"

// Confirm RXX malfunctions on UTF8 strings w/ out '8' set but is
// successful with '8' set
static void 
test0 (void) 
{
  rxx re_utf8_disabled(".", "");
  rxx re_utf8_enabled(".", "8");
  str dog("\xE7\x8B\x97"); 
  assert(!re_utf8_disabled.match(dog));
  assert(re_utf8_enabled.match(dog));
}

int 
main (int argc, char *argv[])
{
  test0();
}
