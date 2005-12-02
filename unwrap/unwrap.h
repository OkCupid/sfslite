// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2005 Max Krohn (email: my last name AT MIT dot ORG)
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
 */

#ifndef _UNWRAP_UNWRAP_H
#define _UNWRAP_UNWRAP_H

/*
 * netdb.h hack
 *
 * There is a conflict betwen flex version 2.5.4 and /usr/include/netdb.h
 * on Linux. flex generates code that #define's __unused to be empty, but
 * in struct gaicb within netdb.h, there is a member field of a struct
 * called __unused, which gets blanked out, causing a compile error.
 * (Note that netdb.h is included from sysconf.h).  Use this hack to
 * not include netdb.h for now...
 */
#ifndef _NETDB_H
# define _SKIP_NETDB_H
# define _NETDB_H
#endif

#include "amisc.h"

#ifdef _SKIP_NETDB_H
# undef _NETDB_H
# undef _SKIP_NETDB_H
#endif
/*
 * end netdb.h hack
 */

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
  str mk_ptr () const;
  str alloc_ptr (const str &nm, const str &args) const;
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
  str initializer () const { return _initializer; }

  str decl () const;
  str decl (const str &prfx, int n) const;
  str ref_decl () const;
  str _name;

protected:
  vartyp_t _asc;
  str _initializer;
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
  void paramlist (strbuf &b, bool types = true) const;
  void initialize (strbuf &b, bool self) const;
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
		     ptr<expr_list_t> e = NULL);
  unwrap_callback_t () : 
    _parent_fn (NULL), _shotgun (NULL), _wrap_in (0),
    _last_wrap_ind (0) {}
  void output (int fd) ;
  void output_in_class (strbuf &b, int n);

  int last_wrap_ind () const { return _last_wrap_ind; }
  int local_cb_ind () const { return _local_cb_ind; }
  int global_cb_ind () const { return _cb_ind; }

  tailq_entry<unwrap_callback_t> _lnk;
  ptr<expr_list_t> wrap_in () { return _wrap_in; }
  ptr<expr_list_t> call_with () { return _call_with; }
private:
  ptr<expr_list_t> _call_with;
  unwrap_fn_t *_parent_fn;
  unwrap_shotgun_t *_shotgun;
  ptr<expr_list_t> _wrap_in;
  int _cb_ind;   // global CB id

  // within a CRCC* environment, all wrapped-in args are considered
  // sequentially, so we need to continue counting across CBs
  int _last_wrap_ind; 

  int _local_cb_ind; // CB id local to this CRCC {... } block
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
  void set_initializer (const str &s) { _initializer = s; }
  void dump () const ;
  str initializer () const { return _initializer; }
private:
  const str _name;
  str _pointer;
  ptr<vartab_t> _params;
  str _initializer;
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

#define STATIC_DECL           (1 << 0)

//
// Unwrap Function Type
//
class unwrap_fn_t : public unwrap_el_t {
public:
  unwrap_fn_t (const str &r, ptr<declarator_t> d, bool c)
    : _ret_type (r, d->pointer ()), _name (d->name ()),
      _name_mangled (mangle (_name)), _method_name (strip_to_method (_name)),
      _class (strip_off_method (_name)), _isconst (c),
      _closure (mk_closure ()), _closure_data (mk_closure_data ()),
      _args (d->params ()), _opts (0), _crcc_star_present (false) {}

  vartab_t *stack_vars () { return &_stack_vars; }
  vartab_t *args () { return _args; }
  vartab_t *class_vars_tmp () { return &_class_vars_tmp; }

  str classname () const { return _class; }
  str name () const { return _name; }
  str signature (bool decl) const;

  void set_opts (int i) { _opts = i; }
  int opts () const { return _opts; }

  void output (int fd);
  void add_callback (unwrap_callback_t *c) { _cbs.insert_tail (c); }
  void add_shotgun (unwrap_shotgun_t *g) ;

  str fn_prefix () const { return _name_mangled; }

  static var_t closure_generic () ;
  str decl_casted_closure (bool do_lhs) const;
  var_t closure () const { return _closure; }
  static var_t trig () ;

  str closure_nm () const { return _closure.name (); }
  str closure_data_nm () const;
  var_t closure_data () const { return _closure_data; }
  str reenter_fn  () const ;
  str frozen_arg (const str &i) const ;

  unwrap_callback_t *last_callback () { return *_cbs.plast; }

  str label (u_int id) const ;

  // either use tmp closure or not, depending on whether or not
  // there was a CRCC* in this function
  str closure_tmp_data () const;
  str closure_tmp () const;
  str closure_tmp_g () const;
  bool need_tmp_closure () const { return /* _crcc_star_present */ true; }
  void hit_crcc_star () { _crcc_star_present = true; }

private:
  const type_t _ret_type;
  const str _name;
  const str _name_mangled;
  const str _method_name;
  const str _class;
  const bool _isconst;
  const var_t _closure;
  const var_t _closure_data;

  var_t mk_closure () const ;
  var_t mk_closure_data () const;

  ptr<vartab_t> _args;
  vartab_t _stack_vars;
  vartab_t _class_vars_tmp;
  tailq<unwrap_callback_t, &unwrap_callback_t::_lnk> _cbs; 
  vec<unwrap_shotgun_t *> _shotguns;

  void output_reenter (strbuf &b);
  void output_closure (int fd);
  void output_closure_data (int fd);
  void output_fn_header (int fd);
  void output_static_decl (int fd);
  void output_stack_vars (strbuf &b);
  void output_jump_tab (strbuf &b);
  
  int _opts;
  bool _crcc_star_present;
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
class unwrap_crcc_star_t;
class parse_state_t {
public:
  parse_state_t () : _crcc_star (NULL) {}
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

  void new_crcc_star (unwrap_crcc_star_t *s);
  unwrap_crcc_star_t *crcc_star () { return _crcc_star; }

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
  unwrap_crcc_star_t *_crcc_star;
  tailq<unwrap_el_t, &unwrap_el_t::_lnk> _elements;
  bool _sym_bit;
  backref_list_t _backrefs;
};

class unwrap_shotgun_t : public parse_state_t, public unwrap_el_t 
{
public:
  unwrap_shotgun_t (unwrap_fn_t *f) :  _fn (f), _id (0) {}
  ~unwrap_shotgun_t () {}

  virtual void output (int fd);
  void set_id (int i) { _id = i; }
  void add_class_var (const var_t &v) { _class_vars.add (v); }

  int add_callback (unwrap_callback_t *cb) 
  { 
    _callbacks.push_back (cb); 
    return _callbacks.size ();
  }

  virtual int advance_wrap_ind (ptr<expr_list_t> l) { return 0; }
  virtual bool output_trig (strbuf &b);
  virtual void output_continuation (unwrap_callback_t *cb, strbuf &b) {}
  virtual void output_resume_struct (strbuf &b) {}
  virtual void output_resume_struct_member (strbuf &b) {}
  virtual bool use_trig () const { return true; }

protected:
  unwrap_fn_t *_fn;
  int _id;
  vartab_t _class_vars;
  vec<unwrap_callback_t *> _callbacks;
};

class unwrap_crcc_star_t : public unwrap_shotgun_t {
public:
  unwrap_crcc_star_t (unwrap_fn_t *f) 
    : unwrap_shotgun_t (f), _nxt_var_ind (0) {}

  bool check_backref (int i);
  void add_resume (const str &s) { _resume = s; }
  void add_backref (int i) { _backrefs.insert (i); }

  // overloaded virtual functions
  int advance_wrap_ind (ptr<expr_list_t> l);
  bool output_trig (strbuf &b);
  void output_continuation (unwrap_callback_t *cb, strbuf &b);
  void output_resume_struct (strbuf &b);
  void output_resume_struct_member (strbuf &b);
  bool use_trig () const { return false; }
  void output (int fd);
  

  str make_resume_ref (bool inclass = false);
  str make_backref (int i, bool inclass = false);
  str make_cb_ind_ref (bool inclass = false);

private:
  int _nxt_var_ind;
  str _resume;
  bhash<int> _backrefs;
};

class expr_list2_t {
public:
  expr_list2_t () {}
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
  u_int             opts;
};
extern YYSTYPE yylval;
extern str filename;

class my_strbuf_t : public strbuf {
public:
  strbuf & mycat (const str &s) { cat (s.cstr (), true); return (*this); }
};


#define CONCAT(in,out)  do { strbuf b; b << in; out = b; } while (0)

#endif /* _UNWRAP_UNWRAP_H */
