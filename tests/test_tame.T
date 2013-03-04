// -*-c++-*-

/**
 * In this test we are only really testing the tame parser itself.
 * The functions in here don't actually do anything we just want to make sure
 * they parse properly and that the resulting code can compile and run.
 */

#include "tame.h"
#include "arpc.h"
#include <iostream>

namespace {
  void array_get(int *v, size_t idx, evi_t ev, CLOSURE);
  void array_get__caller(evi_t evi_t, CLOSURE);
  void run(CLOSURE);

  tamed void array_get(int *v, size_t idx, evi_t ev) {
    tvars {
      int res(v[idx]);
    }
    ev->trigger(res);
  }

  tamed void array_get__caller(evi_t ev) {
    tvars {
      int v[5];
      int res;
    }
    for(int i = 0; i < 5; i++) {
      auto f = [i] (const int j) mutable -> const int { return j + i; };
      v[i] = f(i);
    }
    twait { array_get(v, 4, mkevent(res)); }
    ev->trigger(res);
  }


  tamed void run() {
    tvars { int res(0); }
    twait { array_get__caller(mkevent(res)); };
    assert(res == 8);
    exit(0);
  }

} // namespace

int main (int argc, char *argv[]) {
  run();
  amain();
  return 0;
}
