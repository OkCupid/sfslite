
#include "parseopt.h"
#include "async.h"


int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 2) {
    warnx << "usage: " << progname << " <file>\n";
    exit (-1);
  }
  
  conftab ct;

  str s1, s2, s3;
  int i1 = -1, i2 = -2;
  bool b1 = false, b2 = true;

  s3 = "String3Default";

  ct.add ("StringTest", &s1, "String1Default")
    .add ("IntTest", &i1, 0, 1000, 200)
    .add ("BoolTest", &b1, true)
    .add ("StringTest2", &s2)
    .add ("StringTest3", &s3, NULL)
    .add ("IntTest2", &i2, 30, 100)
    .add ("BoolTest2", &b2);

  if (!ct.run (argv[1], CONFTAB_VERBOSE | CONFTAB_APPLY_DEFAULTS)) {
    warn << argv[1] << ": failed to parse\n";
    exit (1);
  }
  
  warn << "s1: " << (s1 ? s1 : str ("(null)")) << "\n";
  warn << "s2: " << (s2 ? s2 : str ("(null)")) << "\n";
  warn << "s3: " << (s3 ? s3 : str ("(null)")) << "\n";

  warn << "i1: " << i1 << "\n";
  warn << "i2: " << i2 << "\n";
  warn << "b1: " << b1 << "\n";
  warn << "b2: " << b2 << "\n";
}
