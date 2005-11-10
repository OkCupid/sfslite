
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
my_class_t__fn1__reenter (ptr<freezer_t> __frz)
{
  my_class_t__fn1__cnysa_state_t *__frz_c =
    reinterpret_cast<my_class_t__fn1__cnysa_state_t *> (frz);
  __frz_c->self->fn1 (__frz_c->a.a, __frz_c->a.x, __frz_c->a.cb_done,
		      __frz);
}

static void
my_class_t__fn1__cb1 (ptr<freezer_t> __frz, ptr<bool> b, int x)
{
  my_class_t__fn1__cnysa_state_t *__frz_c =
    reinterpret_cast<my_class_t__fn1__cnysa_state_t *> (frz);
  __frz_c->s.b = b;
  __frz_c->s.x = x;
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
	trigger_t::alloc (wrap (my_class_t__fn1__reenter, __frz));

      call_fn (a, x, wrap (my_class_t__fn1__cb1, __frz, trig));
      call_fn2 (a, x, wrap (my_class_t__fn1__cb2, __frz, trig));

      return;
    }
  my_class_t__fn1__label1:
    if (v && b && *b) {
      warn << foo << "\n";
    }
  } else {


  }

}
