
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#ifndef _LIBTAME_CONNECTORS_H_
#define _LIBTAME_CONNECTORS_H_

#include "tame.h"

template<class T1, class T2, class T3> 
class __add_cancel_t {
public:
  __add_cancel_t () {}
  void f (typename event_t<T1,T2,T3>::ptr *out, 
	  cancelable_t *cnc, 
	  typename event_t<T1,T2,T3>::ref in, 
	  bool *res, CLOSURE);
};


template< class T1, class T2, class T3 >
class __add_cancel_t_T1_T2_T3___f__closure_t : public closure_t {
public:
  __add_cancel_t_T1_T2_T3___f__closure_t (__add_cancel_t< T1  ,  T2  ,  T3 > *_self,  typename event_t< T1  ,  T2  ,  T3 >::ptr *out,  cancelable_t *cnc,  typename event_t< T1  ,  T2  ,  T3 >::ref in,  bool *res) : closure_t ("/home/max/brnch/sfslite-1.1/libtame/tame_connectors.Th", "__add_cancel_t< T1  ,  T2  ,  T3 >::f"), _self (_self),  _stack (out, cnc, in, res), _args (out, cnc, in, res) {}
  typedef void  (__add_cancel_t< T1  ,  T2  ,  T3 >::*method_type_t) ( typename event_t< T1  ,  T2  ,  T3 >::ptr *,  cancelable_t *,  typename event_t< T1  ,  T2  ,  T3 >::ref ,  bool *, ptr<closure_t>);
  void set_method_pointer (method_type_t m) { _method = m; }
  void reenter ()
  {
    ((*_self).*_method)  (_args.out, _args.cnc, _args.in, _args.res, mkref (this));
  }
  struct stack_t {
    stack_t ( typename event_t< T1  ,  T2  ,  T3 >::ptr *out,  cancelable_t *cnc,  typename event_t< T1  ,  T2  ,  T3 >::ref in,  bool *res) : rv ("/home/max/brnch/sfslite-1.1/libtame/tame_connectors.Th", 28)  {}
     rendezvous_t< bool > rv;
     T1 t1;
     T2 t2;
     T3 t3;
     bool ok;
  };
  struct args_t {
    args_t ( typename event_t< T1  ,  T2  ,  T3 >::ptr *out,  cancelable_t *cnc,  typename event_t< T1  ,  T2  ,  T3 >::ref in,  bool *res) : out (out), cnc (cnc), in (in), res (res) {}
     typename event_t< T1  ,  T2  ,  T3 >::ptr *out;
     cancelable_t *cnc;
     typename event_t< T1  ,  T2  ,  T3 >::ref in;
     bool *res;
  };
  __add_cancel_t< T1  ,  T2  ,  T3 > *_self;
  stack_t _stack;
  args_t _args;
  method_type_t _method;
  bool is_onstack (const void *p) const
  {
    return (static_cast<const void *> (&_stack) <= p &&
            static_cast<const void *> (&_stack + 1) > p);
  }
};
template< class T1, class T2, class T3 >
void 
__add_cancel_t< T1  ,  T2  ,  T3 >::f( typename event_t< T1  ,  T2  ,  T3 >::ptr *__tame_out,  cancelable_t *__tame_cnc,  typename event_t< T1  ,  T2  ,  T3 >::ref __tame_in,  bool *__tame_res, ptr<closure_t> __cls_g)
{
    __add_cancel_t_T1_T2_T3___f__closure_t< T1  ,  T2  ,  T3 > *__cls;
  ptr<__add_cancel_t_T1_T2_T3___f__closure_t< T1  ,  T2  ,  T3 >  > __cls_r;
  if (!__cls_g) {
    if (tame_check_leaks ()) start_rendezvous_collection ();
    __cls_r = New refcounted<__add_cancel_t_T1_T2_T3___f__closure_t< T1  ,  T2  ,  T3 > > (this, __tame_out, __tame_cnc, __tame_in, __tame_res);
    if (tame_check_leaks ()) __cls_r->collect_rendezvous ();
    __cls = __cls_r;
    __cls_g = __cls_r;
    __cls->set_method_pointer (&__add_cancel_t< T1  ,  T2  ,  T3 >::f);
  } else {
    __cls =     reinterpret_cast<__add_cancel_t_T1_T2_T3___f__closure_t< T1  ,  T2  ,  T3 > *> (static_cast<closure_t *> (__cls_g));
    __cls_r = mkref (__cls);
  }
   rendezvous_t< bool > &rv = __cls->_stack.rv;
   T1 &t1 = __cls->_stack.t1;
   T2 &t2 = __cls->_stack.t2;
   T3 &t3 = __cls->_stack.t3;
   bool &ok = __cls->_stack.ok;
   typename event_t< T1  ,  T2  ,  T3 >::ptr *&out = __cls->_args.out;
   cancelable_t *&cnc = __cls->_args.cnc;
   typename event_t< T1  ,  T2  ,  T3 >::ref &in = __cls->_args.in;
   bool *&res = __cls->_args.res;
   use_reference (out); 
   use_reference (cnc); 
   use_reference (in); 
   use_reference (res); 
  switch (__cls->jumpto ()) {
  case 0: break;
  case 1:
    goto __add_cancel_t_T1_T2_T3___f__label_1;
    break;
  default:
    panic ("unexpected case.\n");
    break;
  }


  cnc->set_notify_on_cancel (mkevent (rv, false));
  *out = mkevent (rv, true, t1, t2, t3);
  __add_cancel_t_T1_T2_T3___f__label_1:
do {
  if (!(rv).next_trigger (ok)) {
    __cls->set_jumpto (1);
      (rv).set_join_cb (wrap (__cls_r, &__add_cancel_t_T1_T2_T3___f__closure_t< T1  ,  T2  ,  T3 > ::reenter));
      return;
  } else {
    (rv).clear_join_method ();
  }
} while (0);

  if (res) *res = ok;
  in->trigger (t1, t2, t3);
  rv.cancel ();
  do {
  __cls->end_of_scope_checks (41);
  return;
  } while (0);
}

template<class T1, class T2, class T3>
typename event_t<T1,T2,T3>::ptr 
add_cancel (cancelable_t *cnc, typename event_t<T1,T2,T3>::ptr in, 
	    bool *res = NULL)
{
  typename event_t<T1,T2,T3>::ptr ncb;
  __add_cancel_t<T1,T2,T3> c;
  c.f (&ncb, cnc, in, res);
  return ncb;
}


#endif /* _LIBAME_CONNECTORS_H_ */
