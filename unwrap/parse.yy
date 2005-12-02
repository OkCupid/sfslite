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
#include "unwrap.h"
#include "parseopt.h"
#define YYSTYPE YYSTYPE
static var_t resolve_variable (const str &s);
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
%token T_FLOAT
%token T_DOUBLE
%token T_SIGNED
%token T_UNSIGNED
%token T_STATIC

%token T_2COLON
%token T_2PCT

/* Keywords for our new filter */
%token T_VARS
%token T_SHOTGUN
%token T_CRCC_STAR
%token T_UNWRAP
%token T_UNWRAP_SD
%token T_RESUME

%token T_2DOLLAR

%type <str> pointer pointer_opt template_instantiation_arg
%type <str> template_instantiation_list template_instantiation
%type <str> template_instantiation_opt typedef_name_single
%type <str> template_instantiation_list_opt identifier
%type <str> typedef_name type_qualifier_list_opt type_qualifier_list
%type <str> type_qualifier type_specifier type_modifier_list
%type <str> type_modifier declaration_specifiers passthrough 
%type <str> expr_var_ref generic_expr cpp_initializer_opt
%type <str> resume_opt resume resume_body resume_ref

%type <decl> init_declarator declarator direct_declarator 
%type <decl> declarator_cpp direct_declarator_cpp


%type <vars> parameter_type_list_opt parameter_type_list parameter_list
%type <exprs> callback_param_list 
%type <pl2> callback_2part_param_list callback_2part_param_list_opt

%type <opt>  const_opt
%type <fn>   fn_declaration

%type <var>  parameter_declaration
%type <var>  callback_param casted_expr callback_class_var
%type <typ>  cast

%type <el>   fn_unwrap vars shotgun callback crcc_star
%type <var>  expr callback_stack_var

%type <opts> fn_open

%%


file:  passthrough 			{ state.passthrough ($1); }
	| file fn passthrough       	{ state.passthrough ($3); }
	;

passthrough: /* empty */	    { $$ = ""; }
	| passthrough T_PASSTHROUGH 
	{
	   strbuf b ($1);
	   b << $2;
	   $$ = str (b);
	}
	;

fn_open: T_UNWRAP			{ $$ = 0; }
	| T_UNWRAP_SD			{ $$ = STATIC_DECL; }
	;

fn:	fn_open '(' fn_declaration ')' 
	{
	   $3->set_opts ($1); 
	   state.new_fn ($3);
	}
	fn_body 
	;

fn_body: '{' fn_statements '}'
	{
	  /* XXX: hack in function body close
	   * better fix: store el_t's in the unwrap_fn_t object
 	   */
	  state.passthrough ("}\n");
	}
	;

/* declaration_specifiers is no longer optional ?! */
fn_declaration: declaration_specifiers declarator const_opt
	{
	   $$ = New unwrap_fn_t ($1, $2, $3);
	}
	;

const_opt: /* empty */		{ $$ = false; }
	| T_CONST		{ $$ = true; }
	;

fn_statements: passthrough			
	{
	  state.passthrough ($1);
	}
	| fn_statements fn_unwrap passthrough
	{
	  state.push ($2);
	  state.passthrough ($3);
	}
	;

fn_unwrap: vars
	| shotgun
	| crcc_star
	;

vars:	T_VARS '{' declaration_list_opt '}'
	{
	  $$ = New unwrap_vars_t ();
	}
	;

shotgun: T_SHOTGUN '{' 
	{
	  unwrap_fn_t *fn = state.function ();
 	  unwrap_shotgun_t *gun = New unwrap_shotgun_t (fn);
	  state.new_shotgun (gun);
	  fn->add_shotgun (gun);
	}
	callbacks_and_passthrough '}'
	{
	  $$ = state.shotgun ();
	}
	;

resume_opt: /* empty */ { $$ = NULL; }
	| resume
	;

resume: T_RESUME '{' resume_body '}'
	{
	  $$ = $3;
	}
	;

resume_body: passthrough
	| resume_body '@' resume_ref passthrough
	{
	  CONCAT($1 << $3 << $4, $$);
	}
	;

resume_ref: T_NUM
	{
 	  unwrap_crcc_star_t *cs = state.crcc_star ();
	  int i;
	  assert (convertint ($1, &i)); 
	  cs->check_backref (i);
 	  cs->add_backref (i);
	  $$ = cs->make_backref (i);
	}
	| '#'
	{
 	  $$ = state.crcc_star ()->make_cb_ind_ref ();
	}
	;

crcc_star: T_CRCC_STAR '{'
	{
	  unwrap_fn_t *fn = state.function ();
 	  unwrap_crcc_star_t *c = New unwrap_crcc_star_t (fn);
	  state.new_shotgun (c);
	  state.new_crcc_star (c);
	  fn->add_shotgun (c);
	  fn->hit_crcc_star ();
	} 
	callbacks_and_passthrough '}' resume_opt
	{
	  state.crcc_star ()->add_resume ($6);
	  $$ = state.shotgun ();
	}
	;

callbacks_and_passthrough: passthrough		
	{ 
	  state.shotgun ()->passthrough ($1); 
	}
	| callbacks_and_passthrough callback passthrough
	{
	  parse_state_t *sg = state.shotgun ();
	  sg->push ($2);
	  sg->passthrough ($3);
	}
	;

callback_2part_param_list_opt:  /* empty */	
	{ 
	  $$ = New refcounted<expr_list2_t> ();
	}
	| callback_2part_param_list  		{ $$ = $1; }
	;

callback_2part_param_list: callback_param_list
	{
	  $$ = New refcounted<expr_list2_t> ($1);
	}
	| callback_param_list ';' 
	{
	  if (state.get_sym_bit ()) {
	     yyerror ("Unexepected $,@ or % expression in wrap arguments");
	  }
	}
	callback_param_list
	{
	  $$ = New refcounted<expr_list2_t> ($1, $4);
	  state.check_backrefs ($4->size ());
	}
	;

callback: '@' '(' 
	{
	   state.clear_backref_list ();
	   state.clear_sym_bit ();
	}
	callback_2part_param_list_opt ')'
	{
 	  unwrap_fn_t *fn = state.function ();
	  unwrap_shotgun_t *g = state.shotgun ();
	  unwrap_callback_t *c = 
   	   New unwrap_callback_t ($4->call_with (), fn, g, $4->wrap_in ());
	  fn->add_callback (c);
	  $$ = c;
	}
	;

callback_param_list: callback_param
	{
	  $$ = New refcounted<expr_list_t> ();
	  $$->push_back ($1);
	}
	| callback_param_list ',' callback_param
	{
	  $1->push_back ($3);
	  $$ = $1;
	}
	;

identifier: T_ID
	;

callback_param: identifier             
	{   
           var_t e = resolve_variable ($1);
	   $$ = e;
	}
	| casted_expr 
	;

casted_expr: cast expr
	{
	   $2.set_type ($1);
	   $$ = $2;

	   if ($$.get_asc () == STACK) {
	      if (state.args ()->exists ($$.name ())) {
	    	strbuf b;
	    	b << "stack variable '" << $$.name () 
                  << "' shadows a parameter";
	        yyerror (b);
	      }
	      if (!state.stack_vars ()->add ($$)) {
	    	strbuf b;
	    	b << "redefinition of stack variable: " << $$.name () ;
	    	yyerror (b);
	      }
	   } else if ($$.get_asc () == CLASS) {
	      state.class_vars_tmp ()->add ($$);
	      state.shotgun ()->add_class_var ($$);
	   }
	}
	;

expr: callback_class_var
	| generic_expr			{ $$ = var_t ($1, EXPR); }
	| callback_stack_var
	;

generic_expr:	passthrough	
	| generic_expr expr_var_ref passthrough
	{
	   state.set_sym_bit ();
	   CONCAT($1 << $2 << $3, $$);
	}
	;

expr_var_ref: T_2PCT		{ $$ = "_self"; }
	| '$' 			{ $$ = "_stack."; }
	| '@' T_NUM		
	{
	  u_int i;
	  assert (convertint ($2, &i));
	  state.add_backref (i, get_yy_lineno ());
	  $$ = str (strbuf ("_a%d", i));
	}
	;

cast: '(' declaration_specifiers pointer_opt ')'
	{
	  $$ = type_t ($2, $3);
	}
	;

callback_stack_var: T_2DOLLAR identifier
	{
	  $$ = var_t ($2, STACK);
	}
	;

callback_class_var: '%' identifier
	{ 
	   $$ = var_t ($2, CLASS);
	}
	;

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
	  if (state.args ()->exists (v.name ())) {
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

cpp_initializer_opt:	/* empty */		{ $$ = NULL; }
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
	   CONCAT($1 << " " << $2, $$);
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
	  CONCAT ($1 << " " << $2, $$);
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
	  CONCAT($1 << " " << $2, $$);
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
	   CONCAT($1 << "::" << $3, $$);
	}
	;

typedef_name_single: identifier template_instantiation_opt
	{
          CONCAT($1 << $2, $$);
	}
	;

template_instantiation_opt: /* empty */ 	{ $$ = ""; }
	| template_instantiation	
	;

template_instantiation: '<' template_instantiation_list_opt '>'
	{
	  CONCAT("<" << $2 << ">", $$);
	}
	;

template_instantiation_list_opt: /* empty */   { $$ = "" ; }
	| template_instantiation_list
	;

template_instantiation_list: template_instantiation_arg
	| template_instantiation_list ',' template_instantiation_arg
	{
	  CONCAT($1 << " , " << $3, $$);
	}
	;

template_instantiation_arg: declaration_specifiers pointer_opt
	{
	  CONCAT($1 << " " << $2, $$);
	}
	;

pointer_opt: /* empty */	{ $$ = ""; }
	| pointer
	;

pointer: '*'			{ $$ = "*"; }
	| '*' type_qualifier_list_opt pointer
	{
	  CONCAT(" * " << $2 << $3, $$);
	}
	;

%%

var_t
resolve_variable (const str &s)
{
   const var_t *e;
   if (!(e = state.stack_vars ()->lookup (s)) && 
       !(e = state.args ()->lookup (s))) {
      strbuf b;
      b << "unbound variable in wrap: " << s;
      yyerror (b);
   }
   return *e;
}
