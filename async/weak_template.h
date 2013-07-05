// -*-c++-*-
#pragma once

// Thanks to Ben Voigt: http://stackoverflow.com/a/17433263/84745
template <typename T> struct Identity { typedef T type; };
#define WEAK_TMPL(T) typename Identity<T>::type
