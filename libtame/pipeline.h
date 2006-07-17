
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#ifndef _LIBTAME_PIPELINE_H_
#define _LIBTAME_PIPELINE_H_

#include "async.h"
#include "tame.h"

template<class W1 = int, class W2 = int, class W3 = int>
class pipeliner_t {
public:
  pipeliner_t (size_t w, size_t n) : 
    _wsz (w),
    _n_calls (n),
    _cg (__FILE__, __LINE__) ,
    _cancelled (false)
  { assert (_wsz > 0); }

  virtual ~pipeliner_t () {}

  void run (cbv done, CLOSURE);
  void cancel () { _cancelled = true; }

protected:
  virtual void process_wv (W1 w1, W2 w2, W3 w3) {}
  virtual cbv::ptr make_cb (size_t i, CLOSURE) = 0;
  virtual void pipeline_op (size_t i, cbv done, CLOSURE) = 0;

  size_t _wsz, _n_calls;
  coordgroup_t<W1,W2,W3> _cg;
  bool _cancelled;

private:
  void wait_n (size_t n, cbv done, CLOSURE);
  void launch (size_t i, cbv done, CLOSURE);
};

template< class W1, class W2, class W3 >
class pipeliner_t_W1_W2_W3___wait_n__closure_t : public closure_t {
public:
  pipeliner_t_W1_W2_W3___wait_n__closure_t (pipeliner_t< W1  ,  W2  ,  W3 > *_self,  size_t n,  cbv done) : closure_t ("/home/max/sfslite2/libtame/pipeline.Th", "pipeliner_t< W1  ,  W2  ,  W3 >::wait_n"), _self (_self),  _stack (n, done), _args (n, done) {}
  typedef void  (pipeliner_t< W1  ,  W2  ,  W3 >::*method_type_t) ( size_t ,  cbv , ptr<closure_t>);
  void set_method_pointer (method_type_t m) { _method = m; }
  void reenter ()
  {
    ((*_self).*_method)  (_args.n, _args.done, mkref (this));
  }
  struct stack_t {
    stack_t ( size_t n,  cbv done) {}
     W1 w1;
     W2 w2;
     W3 w3;
  };
  struct args_t {
    args_t ( size_t n,  cbv done) : n (n), done (done) {}
     size_t n;
     cbv done;
  };
  pipeliner_t< W1  ,  W2  ,  W3 > *_self;
  stack_t _stack;
  args_t _args;
  method_type_t _method;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
template< class W1, class W2, class W3 >
void 
pipeliner_t< W1  ,  W2  ,  W3 >::wait_n( size_t __tame_n,  cbv __tame_done, ptr<closure_t> __cls_g)
{
    pipeliner_t_W1_W2_W3___wait_n__closure_t< W1  ,  W2  ,  W3 > *__cls;
  ptr<pipeliner_t_W1_W2_W3___wait_n__closure_t< W1  ,  W2  ,  W3 >  > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_join_group_collection ();
    __cls_r = New refcounted<pipeliner_t_W1_W2_W3___wait_n__closure_t< W1  ,  W2  ,  W3 > > (this, __tame_n, __tame_done);
    if (tame_check_leaks ()) __cls_r->collect_join_groups ();
    __cls = __cls_r;
    __cls_g = __cls_r;
    __cls->set_method_pointer (&pipeliner_t< W1  ,  W2  ,  W3 >::wait_n);
  } else {
    __cls =     reinterpret_cast<pipeliner_t_W1_W2_W3___wait_n__closure_t< W1  ,  W2  ,  W3 > *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   W1 &w1 = __cls->_stack.w1;
   W2 &w2 = __cls->_stack.w2;
   W3 &w3 = __cls->_stack.w3;
   size_t &n = __cls->_args.n;
   cbv &done = __cls->_args.done;
   use_reference (n); 
   use_reference (done); 
  switch (__cls->jumpto ()) {
  case 0: break;
  case 1:
    goto pipeliner_t_W1_W2_W3___wait_n__label_1;
    break;
  default:
    panic ("unexpected case.\n");
    break;
  }

  while (_cg.n_signals_left () > n) {
    pipeliner_t_W1_W2_W3___wait_n__label_1:
do {
   if (!(_cg).next_var (w1, w2, w3)) {
    __cls->set_jumpto (1);
      (_cg).set_join_cb (wrap (__cls_r, &pipeliner_t_W1_W2_W3___wait_n__closure_t< W1  ,  W2  ,  W3 > ::reenter));
      return;
  }
} while (0);

    process_wv (w1, w2, w3);
  }
  done->signal ();
  __cls->end_of_scope_checks (53);
  return;
}

template< class W1, class W2, class W3 >
class pipeliner_t_W1_W2_W3___launch__closure_t : public closure_t {
public:
  pipeliner_t_W1_W2_W3___launch__closure_t (pipeliner_t< W1  ,  W2  ,  W3 > *_self,  size_t i,  cbv done) : closure_t ("/home/max/sfslite2/libtame/pipeline.Th", "pipeliner_t< W1  ,  W2  ,  W3 >::launch"), _self (_self),  _stack (i, done), _args (i, done) {}
  typedef void  (pipeliner_t< W1  ,  W2  ,  W3 >::*method_type_t) ( size_t ,  cbv , ptr<closure_t>);
  void set_method_pointer (method_type_t m) { _method = m; }
  void reenter ()
  {
    ((*_self).*_method)  (_args.i, _args.done, mkref (this));
  }
  struct stack_t {
    stack_t ( size_t i,  cbv done) {}
  };
  struct args_t {
    args_t ( size_t i,  cbv done) : i (i), done (done) {}
     size_t i;
     cbv done;
  };
  pipeliner_t< W1  ,  W2  ,  W3 > *_self;
  stack_t _stack;
  args_t _args;
  method_type_t _method;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
template< class W1, class W2, class W3 >
void 
pipeliner_t< W1  ,  W2  ,  W3 >::launch( size_t __tame_i,  cbv __tame_done, ptr<closure_t> __cls_g)
{  pipeliner_t_W1_W2_W3___launch__closure_t< W1  ,  W2  ,  W3 > *__cls;
  ptr<pipeliner_t_W1_W2_W3___launch__closure_t< W1  ,  W2  ,  W3 >  > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_join_group_collection ();
    __cls_r = New refcounted<pipeliner_t_W1_W2_W3___launch__closure_t< W1  ,  W2  ,  W3 > > (this, __tame_i, __tame_done);
    if (tame_check_leaks ()) __cls_r->collect_join_groups ();
    __cls = __cls_r;
    __cls_g = __cls_r;
    __cls->set_method_pointer (&pipeliner_t< W1  ,  W2  ,  W3 >::launch);
  } else {
    __cls =     reinterpret_cast<pipeliner_t_W1_W2_W3___launch__closure_t< W1  ,  W2  ,  W3 > *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   size_t &i = __cls->_args.i;
   cbv &done = __cls->_args.done;
   use_reference (i); 
   use_reference (done); 
  switch (__cls->jumpto ()) {
  case 0: break;
  case 1:
    goto pipeliner_t_W1_W2_W3___launch__label_1;
    break;
  default:
    panic ("unexpected case.\n");
    break;
  }

    do {
    __cls->init_block (1, 58);
    __cls->set_jumpto (1);
 wait_n (_wsz - 1, make_cv0 (__cls_g, 1, 58));     if (!__cls->block_dec_count (58))
      return;
  } while (0);
 pipeliner_t_W1_W2_W3___launch__label_1:
    ;

  pipeline_op (i, make_cb (i));
  done->signal ();
  __cls->end_of_scope_checks (61);
  return;
}

template< class W1, class W2, class W3 >
class pipeliner_t_W1_W2_W3___run__closure_t : public closure_t {
public:
  pipeliner_t_W1_W2_W3___run__closure_t (pipeliner_t< W1  ,  W2  ,  W3 > *_self,  cbv done) : closure_t ("/home/max/sfslite2/libtame/pipeline.Th", "pipeliner_t< W1  ,  W2  ,  W3 >::run"), _self (_self),  _stack (done), _args (done) {}
  typedef void  (pipeliner_t< W1  ,  W2  ,  W3 >::*method_type_t) ( cbv , ptr<closure_t>);
  void set_method_pointer (method_type_t m) { _method = m; }
  void reenter ()
  {
    ((*_self).*_method)  (_args.done, mkref (this));
  }
  struct stack_t {
    stack_t ( cbv done) {}
     size_t i;
  };
  struct args_t {
    args_t ( cbv done) : done (done) {}
     cbv done;
  };
  pipeliner_t< W1  ,  W2  ,  W3 > *_self;
  stack_t _stack;
  args_t _args;
  method_type_t _method;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
template< class W1, class W2, class W3 >
void 
pipeliner_t< W1  ,  W2  ,  W3 >::run( cbv __tame_done, ptr<closure_t> __cls_g)
{
    pipeliner_t_W1_W2_W3___run__closure_t< W1  ,  W2  ,  W3 > *__cls;
  ptr<pipeliner_t_W1_W2_W3___run__closure_t< W1  ,  W2  ,  W3 >  > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_join_group_collection ();
    __cls_r = New refcounted<pipeliner_t_W1_W2_W3___run__closure_t< W1  ,  W2  ,  W3 > > (this, __tame_done);
    if (tame_check_leaks ()) __cls_r->collect_join_groups ();
    __cls = __cls_r;
    __cls_g = __cls_r;
    __cls->set_method_pointer (&pipeliner_t< W1  ,  W2  ,  W3 >::run);
  } else {
    __cls =     reinterpret_cast<pipeliner_t_W1_W2_W3___run__closure_t< W1  ,  W2  ,  W3 > *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   size_t &i = __cls->_stack.i;
   cbv &done = __cls->_args.done;
   use_reference (done); 
  switch (__cls->jumpto ()) {
  case 0: break;
  case 1:
    goto pipeliner_t_W1_W2_W3___run__label_1;
    break;
  case 2:
    goto pipeliner_t_W1_W2_W3___run__label_2;
    break;
  default:
    panic ("unexpected case.\n");
    break;
  }

  for (i = 0; i < _n_calls && !_cancelled; i++) {
      do {
    __cls->init_block (1, 70);
    __cls->set_jumpto (1);
 launch (i, make_cv0 (__cls_g, 1, 70));     if (!__cls->block_dec_count (70))
      return;
  } while (0);
 pipeliner_t_W1_W2_W3___run__label_1:
    ;

  }
    do {
    __cls->init_block (2, 72);
    __cls->set_jumpto (2);
 wait_n (0, make_cv0 (__cls_g, 2, 72));     if (!__cls->block_dec_count (72))
      return;
  } while (0);
 pipeliner_t_W1_W2_W3___run__label_2:
    ;

  done->signal ();
  __cls->end_of_scope_checks (74);
  return;
}

class pipeliner_void_t : public pipeliner_t<> {
public:
  pipeliner_void_t (size_t w, size_t s) : pipeliner_t<> (w, s) {}
protected:
  cbv::ptr make_cb (size_t i, CLOSURE);
};

typedef callback<void, size_t, cbb, ptr<closure_t> >::ref pipeline_op_t;

void do_pipeline (size_t w, size_t n, pipeline_op_t op, cbv done, CLOSURE);


#endif /* _LIBTAME_PIPELINE_H_ */


