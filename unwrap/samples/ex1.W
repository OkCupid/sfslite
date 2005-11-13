// -*-c++-*-
/* $Id$ */

#include "unwrap.h"

class sample_t {
public:
  sample_t () {}
  void main_fn (int i, const char *x, cbb::ptr cb, ptr<freezer_t> = NULL);
private:
  void fn1 (int i, callback<void, ptr<int> >::ref cb);
  void fn2 (const char *x, callback<void, bool, str>::ref cb);
  void fn3 (int i, str s, callback<void, str, bool>::ref cb);
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
    str res2;
    bool status2;
    bool ret;
  }
  ret = false;

  SHOTGUN {
    fn1 (i, @($(ptr<int> res1)));
    fn2 (x, @(status2, res2));
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
