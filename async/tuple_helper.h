// -*-c++-*-

#ifndef _ASYNC_TUPLE_HELPER_H_
#define _ASYNC_TUPLE_HELPER_H_

namespace tuple_helper {

// Taken from __tuple in libcxx. Used to do the "indices trick" in variadic
// templates
// http://loungecpp.wikidot.com/tips-and-tricks%3aindices

// make_tuple_indices
template <size_t...> struct tuple_indices {};

template <size_t _Sp, class _IntTuple, size_t _Ep>
struct __make_indices_imp;

template <size_t _Sp, size_t ..._Indices, size_t _Ep>
struct __make_indices_imp<_Sp, tuple_indices<_Indices...>, _Ep> {
    typedef typename __make_indices_imp<_Sp+1,
                                        tuple_indices<_Indices..., _Sp>,
                                        _Ep>::type type;
};

template <size_t _Ep, size_t ..._Indices>
struct __make_indices_imp<_Ep, tuple_indices<_Indices...>, _Ep> {
    typedef tuple_indices<_Indices...> type;
};

template <size_t _Ep, size_t _Sp = 0>
struct make_tuple_indices {
    static_assert(_Sp <= _Ep, "make_tuple_indices input error");
    typedef typename __make_indices_imp<_Sp, tuple_indices<>, _Ep>::type type;
};

template <class _Tp>
typename std::decay<_Tp>::type
decay_copy(_Tp&& __t) {
  return std::forward<_Tp>(__t);
}
}  // namespace tuple_helper

#endif // _ASYNC_TUPLE_HELPER_H_
