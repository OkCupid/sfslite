// -*-c++-*-

/*
 *
 * Copyright (C) 1998-2004 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 * Note: For the purposes of the GNU General Public License, source
 * includes the source to the perl script, not just the resulting
 * header file, callback.h.  However, since the perl script is
 * conveniently embedded in the header file, you needn't do anything
 * special to distribute source as long as you don't take the perl out
 * of the header.
 *
 *
 * WHAT IS THIS?
 *
 * This C++ header file defines the template class callback--a type
 * that approximates function currying.
 *
 * An object of type callback<R, B1, B2> contains a member R operator()
 * (B1, B2)--Thus callbacks are function objects with the first
 * template type specifying the return of the function and the
 * remaining arguments specifying the types of the arguments to pass
 * the function.
 *
 * Callbacks can have any number of arguments up to $NA below.
 * Template arguments that aren't specified default to void and don't
 * need to be passed in.  Thus, a callback<int> acts like a function
 * with signature "int fn ()", a callback<int, char *> acts like a
 * function with signature "int fn (char *)", and so forth.
 *
 * Each callback class has two type members, ptr and ref, specyfing
 * refcounted pointers and references to the callback object
 * respectively.  (See refcnt.h for an explanation of ptr and ref).
 *
 * The function wrap is used to create references to callbacks.  Given
 * a function with signature "R fn (A1, A2, A3)", wrap can generate
 * the following references:
 *
 *    wrap (fn) -> callback<R, A1, A2, A3>::ref
 *    wrap (fn, a1) -> callback<R, A2, A3>::ref
 *    wrap (fn, a1, a2) -> callback<R, A3>::ref
 *    wrap (fn, a1, a2, a3) -> callback<R>::ref
 *
 * When the resulting callback is actually called, it invokes fn.  The
 * argument list it passes fn starts with whatever arguments were
 * initially passed to wrap and then contains whatever arguments are
 * given to the callback object.  For example, given fn above, this
 * code ends up calling "fn (a1, a2, a3)" and assigning the return
 * value to r;
 *
 *    R r;
 *    A1 a1;
 *    A2 a2;
 *    A3 a3;
 *    callback<R, A1>::ptr cb;
 *
 *    cb = wrap (fn, a1);
 *    r = (*cb) (a2, a3);
 *
 * One can create callbacks from class methods as well as global
 * functions.  To do this, simply pass the object as the first
 * parameter to wrap, and the method as the second.  For example:
 *
 * struct foo {
 *   void bar (int, char *);
 *   callback<void, char *>::ref baz () {
 *     return wrap (this, &foo::bar, 7);
 *   }
 * };
 *
 * Note the only way to generate pointers to class members in ANSI C++
 * is with fully qualified member names.  "&foo::bar" cannot be
 * abbreviated to "&bar" in the above example, though most C++
 * compilers still accept that syntax.
 *
 * If wrap is called with a refcounted ref or ptr to a object, instead
 * of a simple pointer, the resulting callback will maintain a
 * reference to the object, ensuring it is not deleted.  For example,
 * in the following code, baz returns a callback with a reference to
 * the current object.  This ensures that a foo will not be deleted
 * until after the callback has been deleted.  Without mkref, if a
 * callback happened after the reference count on a foo object went to
 * zero, the foo object would previously have been deleted and its
 * vtable pointer likely clobbered, resulting in a core dump.
 *
 * struct foo : public virtual refcount {
 *   virtual void bar (int, char *);
 *   callback<void, char *>::ref baz () {
 *     return wrap (mkref (this), &foo::bar, 7);
 *   }
 * };
 *
 * An example:
 *
 * void
 * printstrings (char *a, char *b, char *c)
 * {
 *   printf ("%s %s %s\n", a, b, c);
 * }
 *
 * int
 * main ()
 * {
 *   callback<void, char *>::ref cb1 = wrap (printstrings, "cb1a", "cb1b");
 *   callback<void, char *, char *>::ref cb2 = wrap (printstrings, "cb2a");
 *   callback<void, char *, char *, char *>::ref cb3 = wrap (printstrings);
 *
 *   (*cb1) ("cb1c");                  // prints: cb1a cb1b cb1c
 *   (*cb2) ("cb2b", "cb2c");          // prints: cb2a cb2b cb2c
 *   (*cb3) ("cb3a", "cb3b", "cb3c");  // prints: cb3a cb3b cb3c
 *
 *   return 0;
 * }
 */

#ifndef _CALLBACK1_H_INCLUDED_
#define _CALLBACK1_H_INCLUDED_ 1

#include "refcnt.h"

#define wNew New
#if WRAP_DEBUG
# define callback_line_param \
	const char *dfunc, const char *func, const char *line,
# define wrap_line_param const char *wa1, const char *wa2, \
	const char *func, const char *line,
# define wrap_line_arg wa1, func, line,
# define wrap_c_line_arg wa2, func, line,
# define callback_line_init(super...) super (dfunc, func, line),
# define wrap _wrap
#else /* !WRAP_DEBUG */
# define callback_line_param
# define wrap_line_param
# define wrap_line_arg
# define wrap_c_line_arg
# define callback_line_init(super...)
#endif /* !WRAP_DEBUG */

#if WRAP_USE_NODELETE
#error "We are fuxed"
#endif
#include "tuple_helper.h"
#include <tuple>
#include <type_traits>

template<class R, class... Args>
class callback {
public:
  typedef struct ref<callback<R, Args...>> ref;
  typedef struct ptr<callback<R, Args...>> ptr;

  const char *_closure_type;
  const void *_closure_void;

#if WRAP_DEBUG
  const char *const dest;
  const char *const src;
  const char *const line;
  callback (const char *df, const char *f, const char *l)
    : dest (df[0] == '&' ? df + 1 : df), src (f), line (l) {}
#endif /* WRAP_DEBUG */
  virtual R operator() (Args ...) = 0;
  virtual void trigger (Args ...args) {
      (void) (*this) (args...);
  }
  virtual ~callback () {}
  callback () : _closure_type (NULL), _closure_void (NULL) {}
  void set_gdb_info (const char *typ, const void *v)
    { _closure_type = typ; _closure_void = v; }
};

// callback_a_b
template <class R, class BoundTupple, class... Args> struct partial_apply;

template <typename R, typename... BoundArgs, typename... Args>
struct partial_apply<R, std::tuple<BoundArgs...>, Args...> :
  public callback<R, Args...>{
  typedef R (*cb_t) (BoundArgs..., Args...);
  typedef std::tuple<BoundArgs...> bound_type;
  cb_t f_;
  bound_type bound_;

  typedef typename
    tuple_helper::make_tuple_indices<sizeof...(BoundArgs)>::type indices_;

  template<size_t ..._Indx>
  R apply(tuple_helper::tuple_indices<_Indx...>, Args... args) {
    return f_(std::get<_Indx>(bound_)..., args...);
  }

 public:
  partial_apply(cb_t f, bound_type &&bound) :
    f_(f),
    bound_(std::forward<bound_type>(bound))
    {}

  R operator() (Args... args) override final
  { return apply(indices_(), args...); }

};

// For member pointers....
template <class P, class C, class R, class BoundTupple, class... Args> struct
partial_apply_c;

template <class P, class C, typename R, typename... BoundArgs, typename... Args>
struct partial_apply_c<P, C, R, std::tuple<BoundArgs...>, Args...> :
  public callback<R, Args...>{
  typedef R (C::*cb_t) (BoundArgs..., Args...);
  typedef std::tuple<BoundArgs...> bound_type;
  P c_;
  cb_t f_;
  bound_type bound_;

  typedef typename
    tuple_helper::make_tuple_indices<sizeof...(BoundArgs)>::type indices_;

  template<size_t ..._Indx>
  R apply(tuple_helper::tuple_indices<_Indx...>, Args... args) {
    return ((*c_).*f_) (std::get<_Indx>(bound_)..., args...);
  }

 public:
  partial_apply_c(callback_line_param const P &cc, cb_t f, bound_type &&bound) :
    c_(cc),
    f_(f),
    bound_(std::forward<bound_type>(bound))
    {}

  R operator() (Args... args) override final
  { return apply(indices_(), args...); }

};

template <class P, class C, class R, class BoundTupple, class... Args> struct
partial_apply_c_const;

template <class P, class C, typename R, typename... BoundArgs, typename... Args>
struct partial_apply_c_const<P, C, R, std::tuple<BoundArgs...>, Args...> :
  public callback<R, Args...>{
  typedef R (C::*cb_t) (BoundArgs..., Args...) const;
  typedef std::tuple<BoundArgs...> bound_type;
  P c_;
  cb_t f_;
  bound_type bound_;

  typedef typename
    tuple_helper::make_tuple_indices<sizeof...(BoundArgs)>::type indices_;

  template<size_t ..._Indx>
  R apply(tuple_helper::tuple_indices<_Indx...>, Args... args) {
    return ((*c_).*f_) (std::get<_Indx>(bound_)..., args...);
  }

 public:
  partial_apply_c_const(callback_line_param const P &cc, cb_t f, bound_type &&bound) :
    c_(cc),
    f_(f),
    bound_(std::forward<bound_type>(bound))
    {}

  R operator() (Args... args) override final
  { return apply(indices_(), args...); }

};

// This class is here to help with template argument deduction
// we want to "stage" the deduction for the "partial application" class
// in two steps: deduct BoundArgs based upon the parameters in the closure
// and then R and Arg with the function argument
// The straight forward way fails:
//   template<class R, class... Bound, class... Args>
//   static inline partial_apply<R, std::tuple<Bound...>, Args...>
//   wrap2 (R (*f) (Bound..., Args...), Bound... b) {
//     return partial_apply<R, std::tuple<Bound...>, Args...>(f, b...);
//   }
// Probably because there are two parameter packs to deduce...

template <class... BoundArgs>
struct partial_apply_helper {
  typedef std::tuple<BoundArgs...> bound_t;

  template<class R, class... Args>
  ref<callback<R, Args...>> operator ()
    (R (*f) (BoundArgs..., Args...), BoundArgs... bound) {
    return New refcounted<partial_apply<R, bound_t, Args...>>
      (wrap_line_arg f, std::make_tuple(bound...));
  }
};

template <class P, class C, class... BoundArgs>
struct partial_apply_c_helper {
  typedef std::tuple<BoundArgs...> bound_t;

  template<class R, class... Args>
  ref<callback<R, Args...>> operator ()
    (const P &c, R (C::*f) (BoundArgs..., Args...), BoundArgs... bound) {
    return New refcounted<partial_apply_c<P, C, R, bound_t, Args...>>
      (wrap_line_param c, f, std::make_tuple(bound...));
  }

  template<class R, class... Args>
  ref<callback<R, Args...>> operator ()
    (const P &c, R (C::*f) (BoundArgs..., Args...) const, BoundArgs... bound) {
    return New refcounted<partial_apply_c_const<P, C, R, bound_t, Args...>>
      (wrap_line_param c, f, std::make_tuple(bound...));
  }

};

template <class F, class... BoundArgs>
partial_apply_helper<BoundArgs...> pahelp(F, BoundArgs...) {
  return partial_apply_helper<BoundArgs...>();
}

template <class TP, class P, class C, class... BoundArgs>
partial_apply_c_helper<P, C, BoundArgs...> pahelp(const P&, TP C::*, BoundArgs...) {
  return partial_apply_c_helper<P, C, BoundArgs...>();
}

#define wrap(...) pahelp(__VA_ARGS__)(__VA_ARGS__)

template<class R, class... Args>
class refops<callback<R, Args...>> {
  typedef callback<R, Args...> cb_t;
  REFOPS_DEFAULT (cb_t);

  R operator() (const Args&... a) const
    { return (*p) (a...); }
};

#undef callback_line_param
#undef wrap_line_param
#undef wrap_line_arg
#undef wrap_c_line_arg
#if WRAP_DEBUG
# undef wrap
# define wrap_arg1(arg_1, arg_rest...) #arg_1
# define wrap_arg2(arg_1, arg_rest...) wrap_arg1 (arg_rest)
# define do_wrap(__func_name, __do_wrap_args...) \
	_wrap (wrap_arg1(__do_wrap_args), wrap_arg2(__do_wrap_args), \
	       __func_name, __FL__, ## __do_wrap_args)
# define wrap(__wrap_args...) do_wrap(__PRETTY_FUNCTION__, __wrap_args)
# define gwrap(__wrap_args...) \
  (nodelete_ignore () ? do_wrap(__FL__, __wrap_args) : 0)
#else /* !WRAP_DEBUG */
# define gwrap wrap
#endif /* !WRAP_DEBUG */


#endif /* !_CALLBACK1_H_INCLUDED_ */
