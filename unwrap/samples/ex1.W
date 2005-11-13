// -*-c++-*-
/* $Id$ */

//
// unwrap.h has a definition for the "trig_t" class, whose only purpose
// is to call a callback upon deletion.  So it's especially useful when
// combined with refcounting.
//
#include "unwrap.h"

class sample_t {
public:
  sample_t () {}
  void main_fn (int i, const char *x, cbb::ptr cb, ptr<freezer_t> = NULL);
private:
  void fn1 (int i, callback<void, ptr<int> >::ref cb);
  void fn2 (const char *x, callback<void, bool, str>::ref cb);
  void fn3 (int i, str s, callback<void, str, bool>::ref cb);

  str res2;
};

void
fn1_cb (int i, callback<void, ptr<int> >::ref cb)
{
  (*cb) (New refcounted<int> (i+4));
}

void
sample_t::fn1 (int i, callback<void, ptr<int> >::ref cb)
{
  delaycb (2, 0, wrap (fn1_cb, i, cb));
}

void
fn2_cb (str s, callback<void, bool, str>::ref cb)
{
  strbuf b;
  b << s << ";" << s ;
  (*cb) (true, str (b));

}

void 
sample_t::fn2 (const char *x, callback<void, bool, str>::ref cb)
{
  delaycb (3, 0, wrap (fn2_cb, str (x), cb));
}

void
sample_t::fn3 (int i, str s, callback<void, str, bool>::ref cb)
{
  strbuf b;
  b << s << ": " << i;
  (*cb) (str (b), true);
}



FUNCTION(void sample_t::main_fn (int i, const char *x, cbb::ptr cb))
{
  VARS {
    bool status2;
    bool ret;
  }
  ret = false;

  //
  // Syntax: @(...) is converted into a "wrap" call, and
  //   autogenerates a corresponding callback function.
  //   Within @(...), you can specify "stack" variables from above,
  //   allocate new "stack" variables on the fly with $(...) and
  //   also refrence class variables with %(...).  
  //
  //   In the last case, even though the compiler has enough
  //   information to determine the type of the variable within the
  //   %(..) clause, it can only do so with access to the full type
  //   system.  Since the unwrap tool does not understand types with
  //   this depth, you need to supply a type hint with %(...); so
  //   something like %(str res2) is required to access the member
  //   sample_t::res2 of type str.
  //
  SHOTGUN {
    fn1 (i, @($(ptr<int> res1)));
    fn2 (x, @(status2, %(str res2)));
  }

  if (res1 && status2) {
    SHOTGUN {
      fn3 (*res1, res2, @($(str res3), $(bool status3)));
    }
    if (status3) {
      warn << "got legitimate result: " << res3 << "\n";
      ret = true;
    }
  }
  (*cb) (ret);
}

static void
cb (bool status)
{
  warn << "main_fn returned with status=" << status << "\n";
  exit (0);
}

int
main (int argc, char *arv[])
{
  sample_t s;
  s.main_fn (10, "footime", wrap (cb));
  amain ();
}
