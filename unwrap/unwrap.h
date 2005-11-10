// -*-c++-*-
/* $Id$ */

#ifndef _UNWRAP_H
#define _UNWRAP_H

#include "amisc.h"
#include "vec.h"
#include "union.h"
#include "qhash.h"
#include "list.h"
#include "ihash.h"

extern int yylex ();
extern int yyparse ();
#undef yyerror
extern int yyerror (str);

extern int yyparse ();
extern int yydebug;
extern FILE *yyin;

class unwrap_el_t {
public:
  unwrap_el_t () {}
  virtual ~unwrap_el_t () {}
  virtual bool append (const str &s) { return false; }
  tailq_entry<unwrap_el_t> _lnk;
};

class unwrap_vars_t : public unwrap_el_t {
public:
  unwrap_vars_t () {}
};

class unwrap_passthrough_t : public unwrap_el_t {
public:
  unwrap_passthrough_t (const str &s) { append (s); }
  bool append (const str &s) { strs.push_back (s); buf << s; return true; }
private:
  strbuf buf;
  vec<str> strs;
};

class type_t {
public:
  type_t () {}
  type_t (const str &t, const str &p)
    : _base_type (t), _pointer (p) {}
  str base_type () const { return _base_type; }
  str pointer () const { return _pointer; }
  str to_str () const { return strbuf () << _base_type << " " << _pointer; }
  void set_base_type (const str &t) { _base_type = t; }
  void set_pointer (const str &p) { _pointer = p; }
private:
  str _base_type, _pointer;
};

class declarator_t;
class var_t {
public:
  var_t (const str &t, ptr<declarator_t> d);
  var_t (const str &t, const str &p, const str &n)
    : _type (t, p), _name (n) {}
  var_t () {}
private:
  type_t _type;

public:
  const str &name () const { return _name; }
  type_t *get_type () { return &_type; }
  const type_t * get_type_const () const { return &_type; }

  str _name;
};

class vartab_t {
public:
  ~vartab_t () {}
  vartab_t () {}
  vartab_t (var_t v) { add (v); }
  u_int size () { return _vars.size (); }
  bool add (var_t v) ;

  vec<var_t> _vars;
  qhash<str, u_int> _tab;
};

/*
 * corresponds to the yacc rule for parsing C declarators -- either for
 * variables or for function definitions
 */
class declarator_t {
public:
  declarator_t (const str &n, ptr<vartab_t> v)
    : _name (n), _params (v) {}
  declarator_t (const str &n) : _name (n) {}
  void set_pointer (const str &s) { _pointer = s; }
  str pointer () const { return _pointer; }
  str name () const { return _name; }
  ptr<vartab_t> params () { return _params; }
  void set_params (ptr<vartab_t> v) { _params = v; }
  void dump () const ;
private:
  const str _name;
  str _pointer;
  ptr<vartab_t> _params;
};

// Convert:
//
//   foo_t::max<int,int> => foo_t__max_int_int_
//
str mangle (const str &in);

//
// Unwrap Function Type
//
class unwrap_fn_t : public unwrap_el_t {
public:
  unwrap_fn_t (const str &r, ptr<declarator_t> d, bool c)
    : _ret_type (r, d->pointer ()), _name (d->name ()),
      _name_mangled (mangle (_name)), _isconst (c) {}
  vartab_t *stack_vars () { return &_stack_vars; }
private:
  const type_t _ret_type;
  const str _name;
  const str _name_mangled;
  const bool _isconst;
  vartab_t _args;
  vartab_t _stack_vars;
};

class unwrap_shotgun_t;
class parse_state_t {
public:
  void new_fn (unwrap_fn_t *f) { new_el (f); _fn = f; }
  void new_el (unwrap_el_t *e) { _fn = NULL; _elements.insert_tail (e); }
  void passthrough (const str &s) ;
  void push (unwrap_el_t *e) { _elements.insert_tail (e); }

  vartab_t *stack_vars () { return _fn ? _fn->stack_vars () : NULL; }
  void set_decl_specifier (const str &s) { _decl_specifier = s; }
  str decl_specifier () const { return _decl_specifier; }

  void new_shotgun (unwrap_shotgun_t *g);
  unwrap_shotgun_t *shotgun () { return _shotgun; }

private:
  str _decl_specifier;
  unwrap_fn_t *_fn;
  unwrap_shotgun_t *_shotgun;
  tailq<unwrap_el_t, &unwrap_el_t::_lnk> _elements;
};

class unwrap_shotgun_t : public parse_state_t, public unwrap_el_t 
{
public:
  unwrap_shotgun_t () {}
};

class unwrap_callback_t : public unwrap_el_t {
public:
  

};

extern parse_state_t state;

struct YYSTYPE {
  ::str             str;
  ptr<declarator_t> decl;
  ptr<vartab_t>     vars;
  var_t             var;
  bool              opt;
  char              ch;
  unwrap_fn_t *     fn;
  unwrap_el_t *     el;

  vec<ptr<declarator_t> > decls;
};
extern YYSTYPE yylval;
extern str filename;

#define CONCAT(in,out)  do { strbuf b; b << in; out = b; } while (0)

#endif /* _UNWRAP_H */
