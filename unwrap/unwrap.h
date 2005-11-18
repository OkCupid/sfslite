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
extern int get_yy_lineno ();


typedef enum { NONE = 0, ARG = 1, STACK = 2, CLASS = 3, EXPR = 4 } vartyp_t ;

class unwrap_el_t {
public:
  unwrap_el_t () {}
  virtual ~unwrap_el_t () {}
  virtual bool append (const str &s) { return false; }
  virtual void output (int fd) = 0;
  tailq_entry<unwrap_el_t> _lnk;
};

class unwrap_vars_t : public unwrap_el_t {
public:
  unwrap_vars_t () {}
  void output (int fd) {}
};

class unwrap_passthrough_t : public unwrap_el_t {
public:
  unwrap_passthrough_t (const str &s) { append (s); }
  bool append (const str &s) { _strs.push_back (s); _buf << s; return true; }
  void output (int fd);
private:
  strbuf _buf;
  vec<str> _strs;
};

class type_t {
public:
  type_t () {}
  type_t (const str &t, const str &p)
    : _base_type (t), _pointer (p) {}
  str base_type () const { return _base_type; }
  str pointer () const { return _pointer; }
  str to_str () const;
  void set_base_type (const str &t) { _base_type = t; }
  void set_pointer (const str &p) { _pointer = p; }
  bool is_complete () const { return _base_type; }
private:
  str _base_type, _pointer;
};

class declarator_t;
class var_t {
public:
  var_t () {}
  var_t (const str &n, vartyp_t a = NONE) : _name (n), _asc (a) {}
  var_t (const str &t, ptr<declarator_t> d, vartyp_t a = NONE);
  var_t (const str &t, const str &p, const str &n, vartyp_t a = NONE) : 
    _type (t, p), _name (n), _asc (a) {}
  var_t (const type_t &t, const str &n, vartyp_t a = NONE)
    : _type (t), _name (n), _asc (a) {}
protected:
  type_t _type;

public:
  const type_t &type () const { return _type; }
  const str &name () const { return _name; }
  type_t *get_type () { return &_type; }
  const type_t * get_type_const () const { return &_type; }
  bool is_complete () const { return _type.is_complete (); }

  // ASC = Args, Stack or Class
  void set_asc (vartyp_t a) { _asc = a; }
  vartyp_t get_asc () const { return _asc; }

  void set_type (const type_t &t) { _type = t; }

  str decl () const;
  str decl (const str &prfx, int n) const;
  str ref_decl () const;
  str _name;

protected:
  vartyp_t _asc;
};

typedef vec<var_t> expr_list_t;

class vartab_t {
public:
  ~vartab_t () {}
  vartab_t () {}
  vartab_t (var_t v) { add (v); }
  u_int size () const { return _vars.size (); }
  bool add (var_t v) ;
  void declarations (strbuf &b, const str &padding) const;
  void paramlist (strbuf &b) const;
  bool exists (const str &n) const { return _tab[n]; }
  const var_t *lookup (const str &n) const;

  vec<var_t> _vars;
  qhash<str, u_int> _tab;
};

class unwrap_fn_t; 

class unwrap_shotgun_t;
class unwrap_callback_t : public unwrap_el_t {
public:
  unwrap_callback_t (ptr<expr_list_t> t, unwrap_fn_t *f, unwrap_shotgun_t *g,
		     ptr<expr_list_t> e = NULL) : 
    _call_with (t), _parent_fn (f), _shotgun (g), _wrap_in (e), _cb_id (0) {}
  unwrap_callback_t () : _parent_fn (NULL), _shotgun (NULL), _wrap_in (0) {}
  void output (int fd) ;
  void output_in_class (strbuf &b, int n);
  tailq_entry<unwrap_callback_t> _lnk;
  ptr<expr_list_t> wrap_in () { return _wrap_in; }
  ptr<expr_list_t> call_with () { return _call_with; }
private:
  ptr<expr_list_t> _call_with;
  unwrap_fn_t *_parent_fn;
  unwrap_shotgun_t *_shotgun;
  ptr<expr_list_t> _wrap_in;
  int _cb_id;
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
// convert 
//
//   foo_t::max<int,int>::bar  => bar
//
str strip_to_method (const str &in);
str strip_off_method (const str &in);

class unwrap_shotgun_t;

//
// Unwrap Function Type
//
class unwrap_fn_t : public unwrap_el_t {
public:
  unwrap_fn_t (const str &r, ptr<declarator_t> d, bool c)
    : _ret_type (r, d->pointer ()), _name (d->name ()),
      _name_mangled (mangle (_name)), _method_name (strip_to_method (_name)),
      _class (strip_off_method (_name)), _isconst (c),_closure (mk_closure ()),
      _args (d->params ()) {}

  vartab_t *stack_vars () { return &_stack_vars; }
  vartab_t *args () { return _args; }
  vartab_t *class_vars_tmp () { return &_class_vars_tmp; }

  str classname () const { return _class; }
  str name () const { return _name; }

  void output (int fd);
  void add_callback (unwrap_callback_t *c) { _cbs.insert_tail (c); }
  void add_shotgun (unwrap_shotgun_t *g) { _shotguns.push_back (g); }
  str fn_prefix () const { return _name_mangled; }

  static var_t closure_generic () ;
  str decl_casted_closure (bool do_lhs) const;
  var_t closure () const { return _closure; }
  static var_t trig () ;

  str closure_nm () const { return _closure.name (); }
  str reenter_fn  () const ;
  str frozen_arg (const str &i) const ;

  unwrap_callback_t *last_callback () { return *_cbs.plast; }

  str label (u_int id) const ;


private:
  const type_t _ret_type;
  const str _name;
  const str _name_mangled;
  const str _method_name;
  const str _class;
  const bool _isconst;
  const var_t _closure;

  var_t mk_closure () const ;

  ptr<vartab_t> _args;
  vartab_t _stack_vars;
  vartab_t _class_vars_tmp;
  tailq<unwrap_callback_t, &unwrap_callback_t::_lnk> _cbs; 
  vec<unwrap_shotgun_t *> _shotguns;

  void output_reenter (strbuf &b);
  void output_closure (int fd);
  void output_fn_header (int fd);
  void output_stack_vars (strbuf &b);
  void output_jump_tab (strbuf &b);
  
};

class backref_t {
public:
  backref_t (u_int i, u_int l) : _index (i), _lineno (l) {}
  u_int ref_index () const { return _index; }
  u_int lineno () const { return _lineno; }
protected:
  u_int _index, _lineno;
};

class backref_list_t {
public:
  backref_list_t () {}
  void clear () { _lst.clear (); }
  void add (u_int i, u_int l) { _lst.push_back (backref_t (i, l)); }
  bool check (u_int sz) const;
private:
  vec<backref_t> _lst;
};

class unwrap_shotgun_t;
class parse_state_t {
public:
  void new_fn (unwrap_fn_t *f) { new_el (f); _fn = f; }
  void new_el (unwrap_el_t *e) { _fn = NULL; _elements.insert_tail (e); }
  void passthrough (const str &s) ;
  void push (unwrap_el_t *e) { _elements.insert_tail (e); }

  // access variable tables in the currently active function
  vartab_t *stack_vars () { return _fn ? _fn->stack_vars () : NULL; }
  vartab_t *class_vars_tmp () { return _fn ? _fn->class_vars_tmp () : NULL ; }
  vartab_t *args () { return _fn ? _fn->args () : NULL; }

  void set_decl_specifier (const str &s) { _decl_specifier = s; }
  str decl_specifier () const { return _decl_specifier; }
  unwrap_fn_t *function () { return _fn; }

  void new_shotgun (unwrap_shotgun_t *g);
  unwrap_shotgun_t *shotgun () { return _shotgun; }

  void output (int fd);

  void clear_sym_bit () { _sym_bit = false; }
  void set_sym_bit () { _sym_bit = true; }
  bool get_sym_bit () const { return _sym_bit; }

  void clear_backref_list () { _backrefs.clear (); }
  void add_backref (u_int i, u_int l) { _backrefs.add (i, l); }
  bool check_backrefs (u_int i) const { return _backrefs.check (i); }

protected:
  str _decl_specifier;
  unwrap_fn_t *_fn;
  unwrap_shotgun_t *_shotgun;
  tailq<unwrap_el_t, &unwrap_el_t::_lnk> _elements;
  bool _sym_bit;
  backref_list_t _backrefs;
};

class unwrap_shotgun_t : public parse_state_t, public unwrap_el_t 
{
public:
  unwrap_shotgun_t (unwrap_fn_t *f) : _fn (f), _id (0) {}
  void output (int fd);
  void set_id (int i) { _id = i; }
  void add_class_var (const var_t &v) { _class_vars.add (v); }
private:
  unwrap_fn_t *_fn;
  int _id;
  vartab_t _class_vars;
};

class expr_list2_t {
public:
  expr_list2_t (ptr<expr_list_t> w) : _call_with (w) {}
  expr_list2_t (ptr<expr_list_t> w, ptr<expr_list_t> c) 
    : _wrap_in (w), _call_with (c) {}
  ptr<expr_list_t> wrap_in () { return _wrap_in; }
  ptr<expr_list_t> call_with () { return _call_with; }
private:
  ptr<expr_list_t> _wrap_in;
  ptr<expr_list_t> _call_with;

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
  ptr<expr_list_t>  exprs;
  type_t            typ;
  ptr<expr_list2_t> pl2;
  vec<ptr<declarator_t> > decls;
};
extern YYSTYPE yylval;
extern str filename;

#define CONCAT(in,out)  do { strbuf b; b << in; out = b; } while (0)

#endif /* _UNWRAP_H */
