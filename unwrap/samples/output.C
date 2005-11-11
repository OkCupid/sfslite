
class my_class_t__fn1__cnysa_state_t : public freezer_t {
public:

  struct stack_t {
    int a;
    ptr<vec<bitch_t> > v;
    char *name;
    ptr<bool> b;
    int x;
    int y;
  };

  struct argv_t {
    int a;
    ptr<bar_t> x;
    cbi::ptr cb_done ;
  };

  stack_t s;
  argv_t a;
  my_class_t *self;
};

static void
my_class_t__fn1__freezer_t::reenter ()
{
  self->fn1 (_args.a, _args.x, _args.cb_done, mkref (this));
}

static void
my_class_t__fn1__freezer_t::cb1 (ptr<bool> b, int x, double q, trigger_t trig)
{
  _stack.b = b;
  _stack.x = x;
  _class_tmp.q = q;
}

int
my_class_t::fn1 (int a, ptr<bar_t> x, cbi::ptr cb_done, ptr<freezer_t> __frz)
{
  if (!frz) {
    __frz = New refcounted<my_class_t__fn1__cnysa_state_t> ();
    return fn1 (a, x, cb_done, __frz);
  }

  my_class_t__fn1__cnysa_state_t *__frz_c =
    reinterpret_cast<my_class_t__fn1__cnysa_state_t *> (frz);

  int &i = __frz_c->s.i;
  ptr<vec<bitch_t> > &v =  __frz_c->s.v;
  char *&name = __frz_c->s.name;

  switch (__frz->_jump_to) {
  case 1:
    goto my_class_t__fn1__label1;
    break;
  case 2:
    goto my_class_t__fn1__label2;
    break;
  default:
    break;
  }

  if (a == 10) {

    {
      __frz_c->a.a = a;
      __frz_c->a.x = x;
      __frz_c->a.cb_done = cb_done;
      __frz_c->self = this;
      __frz->_jump_to = 1;

      trigger_t trig = 
	trigger_t::alloc (wrap (__frz, my_class_t__fn1__freezer_t::reenter));

      call_fn (a, x, wrap (__frz, my_class_t__fn1__freezer_t::cb1, trig));
      call_fn2 (a, x, wrap (__frz, my_class_t__fn1__freezer_t::cb2, trig));

      return;
    }
  my_class_t__fn1__label1:
    q = __frz_c->_class_tmp.q;


    if (v && b && *b) {
      warn << foo << "\n";
    }
  } else {


  }

}
