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

#ifndef _TAME_TAME_H
#define _TAME_TAME_H

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

class my_strbuf_t : public strbuf {
public:
  strbuf & mycat (const str &s) { cat (s.cstr (), true); return (*this); }
};


class lstr : public str {
public:
  lstr () : str (), _lineno (0) {}
  lstr (const char *c) : str (c), _lineno (0) {}
  lstr (u_int ln, const str &s) : str (s), _lineno (ln) {}
  lstr (u_int ln) : str (""), _lineno (ln) {}
  void set_lineno (u_int l) { _lineno = l; }
  u_int lineno () const { return _lineno; }
private:
  u_int _lineno;
};

class tame_el_t {
public:
  tame_el_t () {}
  virtual ~tame_el_t () {}
  virtual bool append (const str &s) { return false; }
  virtual void output (int fd) = 0;
  tailq_entry<tame_el_t> _lnk;
};

class element_list_t : public tame_el_t {
public:
  virtual void output (int fd);
  void passthrough (const lstr &l) ;
  void push (tame_el_t *e) { _lst.insert_tail (e); }
protected:
  tailq<tame_el_t, &tame_el_t::_lnk> _lst;
};

class tame_env_t : public element_list_t {
public:
  virtual void output (int fd) { element_list_t::output (fd); }
  virtual bool is_jumpto () const { return false; }
  virtual void set_id (int id) {}
  virtual int id () const { return 0; }
};


class tame_vars_t : public tame_el_t {
public:
  tame_vars_t () {}
  void output (int fd) {}
};

class tame_passthrough_t : public tame_el_t {
public:
  tame_passthrough_t (const lstr &s) { append (s); }
  bool append (const lstr &s) { _strs.push_back (s); _buf << s; return true; }
  void output (int fd);
private:
  strbuf _buf;
  vec<lstr> _strs;
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
  str decl (const str &prfx) const;
  str ref_decl () const;
  str _name;

protected:
  vartyp_t _asc;
  str _initializer;
};

class expr_list_t : public vec<var_t>
{
public:
  void output_vars (strbuf &b, bool first = false, const str &prfx = NULL,
		    const str &sffx = NULL);
};

typedef enum { DECLARATIONS, NAMES, TYPES } list_mode_t;

class vartab_t {
public:
  ~vartab_t () {}
  vartab_t () {}
  vartab_t (var_t v) { add (v); }
  u_int size () const { return _vars.size (); }
  bool add (var_t v) ;
  void declarations (strbuf &b, const str &padding) const;
  void paramlist (strbuf &b, list_mode_t m, str prfx = NULL) const;
  void initialize (strbuf &b, bool self) const;
  bool exists (const str &n) const { return _tab[n]; }
  const var_t *lookup (const str &n) const;

  vec<var_t> _vars;
  qhash<str, u_int> _tab;
};

class tame_fn_t; 

class tame_block_t;
class tame_nonblock_t;
class tame_join_t;

class tame_callback_t : public tame_env_t {
public:
  tame_callback_t (ptr<expr_list_t> l) : _call_with (l) {}

  virtual void output (int fd) { tame_env_t::output (fd); }
  virtual void output_in_class (strbuf &b) = 0;
  virtual void output_generic (strbuf &b) = 0;
  
  ptr<expr_list_t> call_with () { return _call_with; }
protected:
  ptr<expr_list_t> _call_with;
};

class tame_block_callback_t : public tame_callback_t {
public:
  tame_block_callback_t (tame_fn_t *fn, tame_block_t *b, 
			   ptr<expr_list_t> l);
  ~tame_block_callback_t () {}
  void output (int fd);
  void output_in_class (strbuf &b);
  void output_generic (strbuf &b) {}
  int global_cb_ind () const { return _cb_ind; }
private:
  tame_fn_t *_parent_fn;
  tame_block_t *_block;
  int _cb_ind;   // global CB id
};

class tame_nonblock_callback_t : public tame_callback_t {
public:
  tame_nonblock_callback_t (tame_nonblock_t *n, ptr<expr_list_t> l)
    : tame_callback_t (l), _nonblock (n) {}
  void output (int fd);
  void output_in_class (strbuf &b) {}
  void output_generic (strbuf &b);
  str cb_name () const;
private:
  tame_nonblock_t *_nonblock;
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

class tame_block_t;

#define STATIC_DECL           (1 << 0)

//
// Unwrap Function Type
//
class tame_fn_t : public element_list_t {
public:
  tame_fn_t (u_int o, const str &r, ptr<declarator_t> d, bool c, u_int l)
    : _ret_type (r, d->pointer ()), 
      _name (d->name ()),
      _name_mangled (mangle (_name)), 
      _method_name (strip_to_method (_name)),
      _class (strip_off_method (_name)), 
      _self (_class, "*", "_self"),
      _isconst (c),
      _closure (mk_closure ()), 
      _args (d->params ()), 
      _opts (o),
      _lineno (l),
      _n_labels (0)
  { }

  vartab_t *stack_vars () { return &_stack_vars; }
  vartab_t *args () { return _args; }
  vartab_t *class_vars_tmp () { return &_class_vars_tmp; }

  str classname () const { return _class; }
  str name () const { return _name; }
  str signature (bool decl, str prfx = NULL) const;

  void set_opts (int i) { _opts = i; }
  int opts () const { return _opts; }

  bool need_self () const { return (_class && !(_opts & STATIC_DECL)); }

  void jump_out (strbuf &b, int i);

  void output (int fd);

  int add_callback (tame_block_callback_t *c) 
  { _cbs.push_back (c); return _cbs.size (); }

  void add_nonblock_callback (tame_nonblock_callback_t *c)
  { _nbcbs.push_back (c); }

  void add_env (tame_env_t *g) ;

  str fn_prefix () const { return _name_mangled; }

  static var_t closure_generic () ;
  str decl_casted_closure (bool do_lhs) const;
  var_t closure () const { return _closure; }
  static var_t trig () ;

  str closure_nm () const { return _closure.name (); }
  str reenter_fn  () const ;
  str frozen_arg (const str &i) const ;

  tame_callback_t *last_callback () { return _cbs.back (); }

  str label (u_int id) const ;

private:
  const type_t _ret_type;
  const str _name;
  const str _name_mangled;
  const str _method_name;
  const str _class;
  const var_t _self;

  const bool _isconst;
  const var_t _closure;

  var_t mk_closure () const ;

  ptr<vartab_t> _args;
  vartab_t _stack_vars;
  vartab_t _class_vars_tmp;
  vec<tame_block_callback_t *> _cbs; 
  vec<tame_nonblock_callback_t *> _nbcbs;
  vec<tame_env_t *> _envs;

  void output_reenter (strbuf &b);
  void output_closure (int fd);
  void output_fn (int fd);
  void output_static_decl (int fd);
  void output_stack_vars (strbuf &b);
  void output_arg_references (strbuf &b);
  void output_jump_tab (strbuf &b);
  void output_generic (int fd);
  void output_set_method_pointer (my_strbuf_t &b);
  
  int _opts;
  u_int _lineno;
  u_int _n_labels;
};

class parse_state_t : public element_list_t {
public:
  parse_state_t () : _xlate_line_numbers (false),
		     _need_line_xlate (true) 
  {
    _lists.push_back (this);
  }

  void new_fn (tame_fn_t *f) { new_el (f); _fn = f; }
  void new_el (tame_el_t *e) { _fn = NULL; push (e); }

  void passthrough (const lstr &l) { top_list ()->passthrough (l); }
  void push (tame_el_t *e) { top_list ()->push (e); }

  element_list_t *top_list () { return _lists.back (); }
  void push_list (element_list_t *l) { _lists.push_back (l); }
  void pop_list () { _lists.pop_back (); }

  // access variable tables in the currently active function
  vartab_t *stack_vars () { return _fn ? _fn->stack_vars () : NULL; }
  vartab_t *class_vars_tmp () { return _fn ? _fn->class_vars_tmp () : NULL ; }
  vartab_t *args () { return _fn ? _fn->args () : NULL; }

  void set_decl_specifier (const str &s) { _decl_specifier = s; }
  str decl_specifier () const { return _decl_specifier; }
  tame_fn_t *function () { return _fn; }

  void new_block (tame_block_t *g);
  tame_block_t *block () { return _block; }
  void clear_block () { _block = NULL; }

  void new_nonblock (tame_nonblock_t *s);
  tame_nonblock_t *nonblock () { return _nonblock; }

  void new_join (tame_join_t *j);
  tame_join_t *join () { return _join; }

  void output (int fd);

  void clear_sym_bit () { _sym_bit = false; }
  void set_sym_bit () { _sym_bit = true; }
  bool get_sym_bit () const { return _sym_bit; }

  void set_xlate_line_numbers (bool f) { _xlate_line_numbers = f; }
  void set_infile_name (const str &i) { _infile_name = i; }
  void output_line_xlate (int fd, int ln) ;
  void need_line_xlate () { _need_line_xlate = true; }

protected:
  str _decl_specifier;
  tame_fn_t *_fn;
  tame_block_t *_block;
  tame_nonblock_t *_nonblock;
  tame_join_t *_join;
  bool _sym_bit;

  // lists of elements (to reflect nested structure)
  vec<element_list_t *> _lists;

  str _infile_name;
  bool _xlate_line_numbers;
  bool _need_line_xlate;
};

class tame_block_t : public tame_env_t {
public:
  tame_block_t (tame_fn_t *f) : _fn (f), _id (0) {}
  ~tame_block_t () {}
  
  void output (int fd);
  bool is_jumpto () const { return true; }
  void set_id (int i) { _id = i; }
  int id () const { return _id; }
  void add_class_var (const var_t &v) { _class_vars.add (v); }
  
  int add_callback (tame_callback_t *cb) 
  { 
    _callbacks.push_back (cb); 
    return _callbacks.size ();
  }

protected:
  tame_fn_t *_fn;
  int _id;
  vartab_t _class_vars;
  vec<tame_callback_t *> _callbacks;
};
  
class tame_nonblock_t : public tame_env_t {
public:
  tame_nonblock_t (ptr<expr_list_t> l) : _args (l) {}
  ~tame_nonblock_t () {}
  void output (int fd) { element_list_t::output (fd); }
  u_int n_args () const { return _args->size () - 1; }
  var_t join_group () const { return (*_args)[0]; }
  var_t arg (u_int i) const { return (*_args)[i+1]; }
  void output_vars (strbuf &b, bool first, const str &prfx, const str &sffx);
private:
  ptr<expr_list_t> _args;
};

class tame_join_t : public tame_env_t {
public:
  tame_join_t (tame_fn_t *f, ptr<expr_list_t> l) : _fn (f), _args (l) {}
  bool is_jumpto () const { return true; }
  void set_id (int i) { _id = i; }
  int id () const { return _id; }
  void output (int fd);
  var_t join_group () const { return (*_args)[0]; }
  var_t arg (u_int i) const { return (*_args)[i+1]; }
  u_int n_args () const { return _args->size () - 1; }
private:
  tame_fn_t *_fn;
  ptr<expr_list_t> _args;
  int _id;
};

extern parse_state_t state;
extern str infile_name;

struct YYSTYPE {
  ::lstr            str;
  ptr<declarator_t> decl;
  ptr<vartab_t>     vars;
  var_t             var;
  bool              opt;
  char              ch;
  tame_fn_t *     fn;
  tame_el_t *     el;
  ptr<expr_list_t>  exprs;
  type_t            typ;
  vec<ptr<declarator_t> > decls;
  u_int             opts;
};
extern YYSTYPE yylval;
extern str filename;

#define CONCAT(ln,in,out)                                 \
do {                                                      \
      strbuf b;                                           \
      b << in;                                            \
      out = lstr (ln, b);                                 \
} while (0)



#endif /* _TAME_TAME_H */
