/* -*-fundamental-*- */
/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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

%{
#include "tame.h"
#include "parseopt.h"
#define YYSTYPE YYSTYPE
%}

%token <str> T_ID
%token <str> T_NUM
%token <str> T_PASSTHROUGH

/* Tokens for C++ Variable Modifiers */
%token T_CONST
%token T_STRUCT
%token T_TYPENAME
%token T_VOID
%token T_CHAR
%token T_SHORT
%token T_INT
%token T_LONG
%token T_LONG_LONG
%token T_FLOAT
%token T_DOUBLE
%token T_SIGNED
%token T_UNSIGNED
%token T_STATIC

%token T_2COLON
%token T_RETURN
%token T_UNBLOCK
%token T_RESUME

/* Keywords for our new filter */
%token T_TAME
%token T_VARS
%token T_BLOCK
%token T_NONBLOCK
%token T_JOIN

%token T_2DOLLAR

%type <str> pointer pointer_opt template_instantiation_arg
%type <str> template_instantiation_list template_instantiation
%type <str> template_instantiation_opt typedef_name_single
%type <str> template_instantiation_list_opt identifier
%type <str> typedef_name type_qualifier_list_opt type_qualifier_list
%type <str> type_qualifier type_specifier type_modifier_list
%type <str> type_modifier declaration_specifiers passthrough expr
%type <str> cpp_initializer_opt

%type <decl> init_declarator declarator direct_declarator 
%type <decl> declarator_cpp direct_declarator_cpp


%type <vars> parameter_type_list_opt parameter_type_list parameter_list
%type <exprs> expr_list join_list id_list_opt id_list expr_list_opt

%type <opt>  const_opt
%type <fn>   fn_declaration

%type <var>  parameter_declaration

%type <el>   fn_tame vars block nonblock callback join return_statement 
%type <el>   resume_statement
%type <ret>  resume_keyword

%type<str>   resume_arg_list resume_arg_list_opt

%type <opts> static_opt

%%


file:  passthrough 			{ state.passthrough ($1); }
	| file fn passthrough       	{ state.passthrough ($3); }
	;

passthrough: /* empty */	    { $$ = lstr (get_yy_lineno ()); }
	| passthrough T_PASSTHROUGH 
	{
	   strbuf b ($1);
	   b << $2;
	   $$ = lstr ($1.lineno (), b);
	}
	;

expr:	T_PASSTHROUGH		   { $$ = lstr (get_yy_lineno (), $1); }
	| expr T_PASSTHROUGH
	{
	   strbuf b ($1);
	   b << $2;
	   $$ = lstr ($1.lineno (), b);
	}
	;

fn:	T_TAME '(' fn_declaration ')' '{'
	{
	  state.new_fn ($3);
	  state.push_list ($3);
	}
	fn_statements '}'
	{
	  state.push (New tame_fn_return_t (get_yy_lineno (), 
				            state.function ()));
	  state.passthrough (lstr (get_yy_lineno (), "}"));
	  state.pop_list ();
	}
	;

static_opt: T_STATIC 	{ $$ = STATIC_DECL; }
	| /* empty */	{ $$ = 0; }
	;

/* declaration_specifiers is no longer optional ?! */
fn_declaration: static_opt declaration_specifiers declarator const_opt
	{
	   $$ = New tame_fn_t ($1, $2, $3, $4, get_yy_lineno ());
	}
	;

const_opt: /* empty */		{ $$ = false; }
	| T_CONST		{ $$ = true; }
	;

fn_statements: passthrough			
	{
	  state.passthrough ($1);
	}
	| fn_statements fn_tame passthrough
	{
	  state.push ($2);
	  state.passthrough ($3);
	}
	;

fn_tame: vars
	| block
	| nonblock
	| join
	| return_statement
	| resume_statement
	;

vars:	T_VARS '{' declaration_list_opt '}'
	{
	  $$ = New tame_vars_t ();
	}
	;

resume_keyword:	
	 T_UNBLOCK { $$ = New tame_unblock_t (get_yy_lineno (), 
	                                      state.function ()); }

	| T_RESUME  { $$ = New tame_resume_t (get_yy_lineno (), 
	                                      state.function ()); }
	;

resume_arg_list: '(' passthrough ')' 
	{
	  $$ = $2;
	}
	;

resume_arg_list_opt: /* empty */  { $$ = lstr (get_yy_lineno ());  }
	| resume_arg_list
	;

resume_statement: resume_keyword resume_arg_list_opt ';'
	{
	  if ($2)
	    $1->add_params ($2);
	  $1->passthrough (lstr (get_yy_lineno (), ";"));
	  $$ = $1;
	}
	;

return_statement: T_RETURN passthrough ';'
	{
	   tame_ret_t *r = New tame_ret_t (get_yy_lineno (), 
			  	    state.function ());	
	   if ($2)
	     r->add_params ($2);
 	   r->passthrough (lstr (get_yy_lineno (), ";"));
	   $$ = r;
	}
	;

block: T_BLOCK '{' 
	{
	  tame_fn_t *fn = state.function ();
 	  tame_block_t *bl = New tame_block_t (fn);
	  state.new_block (bl);
	  fn->add_env (bl);
	  fn->hit_tame_block ();
	  state.push_list (bl);
	}
	callbacks_and_passthrough '}'
	{
 	  state.pop_list ();
	  $$ = state.block ();
	  state.clear_block ();
	}
	;

id_list_opt: /* empty */  { $$ = NULL; }
	| id_list
	;

id_list:  ',' identifier
	{
	  $$ = New refcounted<expr_list_t> ();
	  $$->push_back (var_t ()); // reserve 1 empty spot!
	  $$->push_back (var_t ($2, STACK));
	}
	| id_list ',' identifier
	{
	  $1->push_back (var_t ($3, STACK));
	  $$ = $1;
	}
	;

join_list: passthrough id_list_opt
	{
	  if ($2) {
	    (*$2)[0] = var_t ($1, EXPR);
	    $$ = $2;
	  } else {
	    $$ = New refcounted<expr_list_t> ();
	    $$->push_back (var_t ($1, EXPR));
	  }
	}
	;

expr_list_opt:	/* empty */	{ $$ = New refcounted<expr_list_t> (); }
	| expr_list
	;

expr_list: expr
	{
	  $$ = New refcounted<expr_list_t> ();
	  $$->push_back (var_t ($1, EXPR));
	}
	|
	expr_list ',' expr
	{
	  $1->push_back (var_t ($3, EXPR));
	  $$ = $1;
	}
	;

nonblock: T_NONBLOCK '(' expr_list ')' '{'
	{
	  tame_fn_t *fn = state.function ();
 	  tame_nonblock_t *c = New tame_nonblock_t ($3);
	  state.new_nonblock (c);
	  fn->add_env (c);
	  state.push_list (c);
	  state.passthrough (lstr (get_yy_lineno (), "{"));
	} 
	callbacks_and_passthrough '}'
	{
	  state.passthrough (lstr (get_yy_lineno (), "}"));
	  state.pop_list ();
	  $$ = state.nonblock ();
	}
	;

join: T_JOIN '(' join_list ')' '{' 
	{
	  tame_fn_t *fn = state.function ();
	  tame_join_t *jn = New tame_join_t (fn, $3);
	  state.new_join (jn);
	  fn->add_env (jn);
	  state.passthrough (lstr (get_yy_lineno (), "{"));
	  state.push_list (jn);
	}
	fn_statements '}'
	{
	  state.pop_list ();
	  state.passthrough (lstr (get_yy_lineno (), "}"));
	  $$ = state.join ();
	}
	;

callbacks_and_passthrough: passthrough		
	{ 
	  state.passthrough ($1); 
	}
	| callbacks_and_passthrough callback passthrough
	{
	  state.push ($2);
	  state.passthrough ($3);
	}
	;

callback: '@' '(' expr_list_opt ')'
	{
  	  tame_fn_t *fn = state.function ();
	  if (state.block ()) {
 	    // Callbacks are labeled serially within a function; the 
	    // constructor sets this ID.
	    $$ = New tame_block_callback_t (get_yy_lineno (), 
				            fn, state.block (), $3);
	  } else {
	    assert ( state.nonblock ());
	    tame_nonblock_callback_t *cb = 
              New tame_nonblock_callback_t (get_yy_lineno (), 
				            state.nonblock (), $3);
	    fn->add_nonblock_callback (cb);
	    $$ = cb;
	  }
	}
	;

identifier: T_ID
	;

/*
 * No longer need casts..
 *
cast: '(' declaration_specifiers pointer_opt ')'
	{
	  $$ = type_t ($2, $3);
	}
	;
 */

declaration_list_opt: /* empty */
	| declaration_list
	;

declaration_list: declaration
	| declaration_list declaration
	;

parameter_type_list_opt: /* empty */ { $$ = NULL; }
	| parameter_type_list
	;

/* missing: '...'
 */
parameter_type_list: parameter_list
	;

parameter_list: parameter_declaration 
	{
	  $$ = New refcounted<vartab_t> ($1);
	}
	| parameter_list ',' parameter_declaration
	{
	  if (! ($1)->add ($3) ) {
	    strbuf b;
	    b << "duplicated parameter in param list: " << $3.name ();
	    yyerror (b);
          } else {
 	    $$ = $1;
          }
	}
	;

/* missing: abstract declarators
 */
parameter_declaration: declaration_specifiers declarator
	{
	  if ($2->params ()) {
	    warn << "parameters found when not expected\n";
	  }
	  $$ = var_t ($1, $2, ARG);
	}
	;

declaration: declaration_specifiers 
	{
	  state.set_decl_specifier ($1);
	}
	init_declarator_list_opt ';'
	;

init_declarator_list_opt: /* empty */
	| init_declarator_list
	;

init_declarator_list:  init_declarator
	| init_declarator_list ',' init_declarator
	;

/* missing: C++-style initialization, C-style initiatlization
 */
init_declarator: declarator_cpp cpp_initializer_opt
	{
	  if ($2 && $2.len () > 0) 
	    $1->set_initializer ($2);

	  vartab_t *t = state.stack_vars ();

	  var_t v (state.decl_specifier (), $1, STACK);
	  if (state.args () &&
              state.args ()->exists (v.name ())) {
	    strbuf b;
	    b << "stack variable '" << v.name () << "' shadows a parameter";
	    yyerror (b);
	  }
	  if (!t->add (v)) {
	    strbuf b;
	    b << "redefinition of stack variable: " << v.name () ;
 	    yyerror (b);
          }
	}
	;

declarator: pointer_opt direct_declarator
	{
	  if ($1.len () > 0) 
	    $2->set_pointer ($1);
  	  $$ = $2;
	}
	;

declarator_cpp: pointer_opt direct_declarator_cpp
	{
	  if ($1.len () > 0) 
	    $2->set_pointer ($1);
  	  $$ = $2;
	}
	;

cpp_initializer_opt:	/* empty */ { $$ = lstr (get_yy_lineno (), NULL); }
	| '(' passthrough ')'
	{
	  $$ = $2;
	}
	;

direct_declarator_cpp:	identifier 
	{
	  $$ = New refcounted<declarator_t> ($1);
	}
	;

/* 
 * use "typedef_name" instead of identifier for C++-style names
 *
 * simplified to not be recursive...
 */
direct_declarator: typedef_name
	{
	   $$ = New refcounted<declarator_t> ($1);
	}
	| typedef_name '(' parameter_type_list_opt ')'
	{
	   $$ = New refcounted<declarator_t> ($1, $3);
	}
	;


/* missing: first rule:
 *	storage_class_specifier declaration_specifiers_opt
 *
 * changed rule, to eliminate s/r conflicts
 *
 * Returns: <str> element, with the type of the variable (unparsed)
 */
declaration_specifiers: type_modifier_list type_specifier
	{
	   CONCAT($1.lineno (), $1 << " " << $2, $$);
	}
	;

/*
 * new rule to eliminate s/r conflicts
 */
type_modifier:  type_qualifier
	| T_SIGNED		{ $$ = "signed"; }
	| T_UNSIGNED		{ $$ = "unsigned"; }
	;

type_modifier_list: /* empty */ { $$ = ""; }
	| type_modifier_list type_modifier
	{
	  CONCAT ($1.lineno (), $1 << " " << $2, $$);
	}
	;
	

/* missing: struct and enum rules:
 *	| struct_or_union_specifier
 *	| enum_specifier
 */
type_specifier: T_VOID		{ $$ = "void" ; }
	| T_CHAR 		{ $$ = "char";  }
	| T_SHORT		{ $$ = "short"; }
	| T_INT			{ $$ = "int" ; }
	| T_LONG		{ $$ = "long" ; }
	| T_LONG_LONG		{ $$ = "long long"; }
	| T_FLOAT		{ $$ = "float"; }
	| T_DOUBLE		{ $$ = "double" ; }
	| typedef_name
	;

/*
 * hack for now -- not real C syntax
 */
type_qualifier:	T_CONST		{ $$ = "const"; }
	| T_STRUCT		{ $$ = "struct"; }
	;

type_qualifier_list: type_qualifier
	| type_qualifier_list type_qualifier
	{
	  CONCAT($1.lineno (), $1 << " " << $2, $$);
	}
	;

type_qualifier_list_opt: /* empty */ { $$ = ""; }
	| type_qualifier_list
	;

/*
 * foo<int, char *>::bar_t::my_class<int> -> 
 *   foo<> bar_t my_class<>
 */
typedef_name:  typedef_name_single
	| typedef_name T_2COLON typedef_name_single
	{
	   CONCAT($1.lineno (), $1 << "::" << $3, $$);
	}
	;

typedef_name_single: identifier template_instantiation_opt
	{
          CONCAT($1.lineno (), $1 << $2, $$);
	}
	;

template_instantiation_opt: /* empty */ 	{ $$ = ""; }
	| template_instantiation	
	;

template_instantiation: '<' template_instantiation_list_opt '>'
	{
	  CONCAT($2.lineno (), "<" << $2 << ">", $$);
	}
	;

template_instantiation_list_opt: /* empty */   { $$ = "" ; }
	| template_instantiation_list
	;

template_instantiation_list: template_instantiation_arg
	| template_instantiation_list ',' template_instantiation_arg
	{
	  CONCAT($1.lineno (), $1 << " , " << $3, $$);
	}
	;

template_instantiation_arg: declaration_specifiers pointer_opt
	{
	  CONCAT($1.lineno (), $1 << " " << $2, $$);
	}
	;

pointer_opt: /* empty */	{ $$ = ""; }
	| pointer
	;

pointer: '*'			{ $$ = "*"; }
	| '*' type_qualifier_list_opt pointer
	{
	  CONCAT($2.lineno (), " * " << $2 << $3, $$);
	}
	;

%%

