#include "unwrap.h"
#include "async.h"

TAME(static void foo())
{}

int
main (int argc, char *argv[])
{
  foo ();
}
