
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#include "pipeline.h"

static void
__nonblock_cb_0_0 (ptr<closure_t> hold, ptr<joiner_t<> > j,
		value_set_t<> w)
{
  j->join (w);
}
template<class G>
static cbv
__make_cv_0_0 (ptr<closure_t> c, const char *loc, G &g)
{
  g.launch_one (c);
  return wrap (__nonblock_cb_0_0,
	c, g.make_joiner (loc),
	value_set_t<> ());
}
class pipeliner_void_t__make_cb__closure_t : public closure_t {
public:
  pipeliner_void_t__make_cb__closure_t (pipeliner_void_t *_self,  size_t i) : closure_t ("/home/max/sfslite2/libtame/pipeline.T", "pipeliner_void_t::make_cb"), _self (_self),  _stack (i), _args (i) {}
  typedef cbv::ptr  (pipeliner_void_t::*method_type_t) ( size_t , ptr<closure_t>);
  void set_method_pointer (method_type_t m) { _method = m; }
  void reenter ()
  {
    ((*_self).*_method)  (_args.i, mkref (this));
  }
  struct stack_t {
    stack_t ( size_t i) {}
     cbv::ptr ret;
  };
  struct args_t {
    args_t ( size_t i) : i (i) {}
     size_t i;
  };
  pipeliner_void_t *_self;
  stack_t _stack;
  args_t _args;
  method_type_t _method;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
cbv::ptr 
pipeliner_void_t::make_cb( size_t __tame_i, ptr<closure_t> __cls_g)
{
    pipeliner_void_t__make_cb__closure_t *__cls;
  ptr<pipeliner_void_t__make_cb__closure_t > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_join_group_collection ();
    __cls_r = New refcounted<pipeliner_void_t__make_cb__closure_t> (this, __tame_i);
    if (tame_check_leaks ()) __cls_r->collect_join_groups ();
    __cls = __cls_r;
    __cls_g = __cls_r;
    __cls->set_method_pointer (&pipeliner_void_t::make_cb);
  } else {
    __cls =     reinterpret_cast<pipeliner_void_t__make_cb__closure_t *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   cbv::ptr &ret = __cls->_stack.ret;
   size_t &i = __cls->_args.i;
   use_reference (i); 
  switch (__cls->jumpto ()) {
  case 0: break;
  default:
    panic ("unexpected case.\n");
    break;
  }

  
  ret = __make_cv_0_0(__cls_g, "/home/max/sfslite2/libtame/pipeline.T:12: in function pipeliner_void_t::make_cb", _cg);
  __cls->end_of_scope_checks (13);
  do {  return ret; } while (0);
}

class pipeliner_cb_t : public pipeliner_void_t {
public:
  pipeliner_cb_t (size_t w, size_t n, pipeline_op_t o)
    : pipeliner_void_t (w, n), _op (o) {}
  void pipeline_op (size_t i, cbv cb, CLOSURE);
private:
  pipeline_op_t _op;

};

class pipeliner_cb_t__pipeline_op__closure_t : public closure_t {
public:
  pipeliner_cb_t__pipeline_op__closure_t (pipeliner_cb_t *_self,  size_t i,  cbv cb) : closure_t ("/home/max/sfslite2/libtame/pipeline.T", "pipeliner_cb_t::pipeline_op"), _self (_self),  _stack (i, cb), _args (i, cb) {}
  typedef void  (pipeliner_cb_t::*method_type_t) ( size_t ,  cbv , ptr<closure_t>);
  void set_method_pointer (method_type_t m) { _method = m; }
  void reenter ()
  {
    ((*_self).*_method)  (_args.i, _args.cb, mkref (this));
  }
  struct stack_t {
    stack_t ( size_t i,  cbv cb) {}
     bool ok;
  };
  struct args_t {
    args_t ( size_t i,  cbv cb) : i (i), cb (cb) {}
     size_t i;
     cbv cb;
  };
  pipeliner_cb_t *_self;
  stack_t _stack;
  args_t _args;
  method_type_t _method;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
void 
pipeliner_cb_t::pipeline_op( size_t __tame_i,  cbv __tame_cb, ptr<closure_t> __cls_g)
{
    pipeliner_cb_t__pipeline_op__closure_t *__cls;
  ptr<pipeliner_cb_t__pipeline_op__closure_t > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_join_group_collection ();
    __cls_r = New refcounted<pipeliner_cb_t__pipeline_op__closure_t> (this, __tame_i, __tame_cb);
    if (tame_check_leaks ()) __cls_r->collect_join_groups ();
    __cls = __cls_r;
    __cls_g = __cls_r;
    __cls->set_method_pointer (&pipeliner_cb_t::pipeline_op);
  } else {
    __cls =     reinterpret_cast<pipeliner_cb_t__pipeline_op__closure_t *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   bool &ok = __cls->_stack.ok;
   size_t &i = __cls->_args.i;
   cbv &cb = __cls->_args.cb;
   use_reference (i); 
   use_reference (cb); 
  switch (__cls->jumpto ()) {
  case 0: break;
  case 1:
    goto pipeliner_cb_t__pipeline_op__label_1;
    break;
  default:
    panic ("unexpected case.\n");
    break;
  }

    do {
    __cls->init_block (1, 31);
    __cls->set_jumpto (1);
 (*_op) (i, make_cv1 (__cls_g, 1, 31, ok), NULL);     if (!__cls->block_dec_count (31))
      return;
  } while (0);
 pipeliner_cb_t__pipeline_op__label_1:
    ;

  if (!ok) cancel ();
  cb->signal ();
  __cls->end_of_scope_checks (34);
  return;
}

class do_pipeline__closure_t : public closure_t {
public:
  do_pipeline__closure_t ( size_t w,  size_t n,  pipeline_op_t op,  cbv done) : closure_t ("/home/max/sfslite2/libtame/pipeline.T", "do_pipeline"),  _stack (w, n, op, done), _args (w, n, op, done) {}
  void reenter ()
  {
    do_pipeline (_args.w, _args.n, _args.op, _args.done, mkref (this));
  }
  struct stack_t {
    stack_t ( size_t w,  size_t n,  pipeline_op_t op,  cbv done) : ppl (w, n, op)  {}
     pipeliner_cb_t ppl;
  };
  struct args_t {
    args_t ( size_t w,  size_t n,  pipeline_op_t op,  cbv done) : w (w), n (n), op (op), done (done) {}
     size_t w;
     size_t n;
     pipeline_op_t op;
     cbv done;
  };
  stack_t _stack;
  args_t _args;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
void 
do_pipeline( size_t __tame_w,  size_t __tame_n,  pipeline_op_t __tame_op,  cbv __tame_done, ptr<closure_t> __cls_g)
{
    do_pipeline__closure_t *__cls;
  ptr<do_pipeline__closure_t > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_join_group_collection ();
    __cls_r = New refcounted<do_pipeline__closure_t> (__tame_w, __tame_n, __tame_op, __tame_done);
    if (tame_check_leaks ()) __cls_r->collect_join_groups ();
    __cls = __cls_r;
    __cls_g = __cls_r;
  } else {
    __cls =     reinterpret_cast<do_pipeline__closure_t *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   pipeliner_cb_t &ppl = __cls->_stack.ppl;
   size_t &w = __cls->_args.w;
   size_t &n = __cls->_args.n;
   pipeline_op_t &op = __cls->_args.op;
   cbv &done = __cls->_args.done;
   use_reference (w); 
   use_reference (n); 
   use_reference (op); 
   use_reference (done); 
  switch (__cls->jumpto ()) {
  case 0: break;
  case 1:
    goto do_pipeline__label_1;
    break;
  default:
    panic ("unexpected case.\n");
    break;
  }

    do {
    __cls->init_block (1, 42);
    __cls->set_jumpto (1);
 ppl.run (make_cv0 (__cls_g, 1, 42));     if (!__cls->block_dec_count (42))
      return;
  } while (0);
 do_pipeline__label_1:
    ;

  done->signal ();
  __cls->end_of_scope_checks (44);
  return;
}
