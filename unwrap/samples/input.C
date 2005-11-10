


FUNCTION(int my_class_t::fn1 (int a, ptr<T> x, cbi::ptr cb_done))
{
  VARS {
    int i;
    const ptr<vec<bitch_t> > **v;
    char *name;
    ptr<bool> b;
    int x, y;
  }
  
  if (a == 10) {
    SHOTGUN {
      call_fn (a, x, CB(b,x));
      call_fn2 (a,x, CB(v,name));
    }
    if (v && b && *b) {
      warn << foo << "\n";
    }
  } else {
    SHOTGUN {
      call_fn3 (a, x, @(y, $(int x), %(double q)), bitch (x));
    }
  }
  (*cb_done) (y);
};
