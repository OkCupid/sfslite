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
%token T_FLOAT
%token T_DOUBLE
%token T_SIGNED
%token T_UNSIGNED
%token T_STATIC

%token T_2COLON

/* Keywords for our new filter */
%token T_VARS
%token T_SHOTGUN
%token T_MEMBER
%token T_FUNCTION

%token T_2COLON

%type <str> pointer pointer_opt template_instantiation_arg
%type <str> template_instantiation_list template_instantiation
%type <str> template_instantiation_opt typedef_name_single
%type <str> template_instantiation_list_opt identifier
%type <str> typedef_name type_qualifier_list_opt type_qualifier_list
%type <str> type_qualifier type_specifier type_modifier_list
%type <str> type_modifier declaration_specifiers passthrough

%type <decl> init_declarator declarator direct_declarator

%type <vars> parameter_type_list_opt parameter_type_list parameter_list
%type <vars> callback_param_list callback_param_list_opt

%type <opt>  const_opt
%type <fn>   fn_declaration

%type <var>  parameter_declaration callback_class_var callback_stack_var
%type <var>  callback_param

%type <el>   fn_unwrap vars shotgun callback

%%


file:  passthrough
	| file fn passthrough
	;

passthrough: /* empty */	    { $$ = ""; }
	| passthrough T_PASSTHROUGH 
	{
	   strbuf b ($1);
	   b << $2;
	   $$ = str (b);
	}
	;

fn:	T_FUNCTION '(' fn_declaration ')' 
	{
	   state.new_fn ($3);
	}
	fn_body 
	;

fn_body: '{' fn_statements '}'
	;

/* declaration_specifiers is no longer optional ?! */
fn_declaration: declaration_specifiers declarator const_opt
	{
	   $2->dump ();
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
	;

vars:	T_VARS '{' declaration_list_opt '}'
	{
	  $$ = New unwrap_vars_t ();
	}
	;

shotgun: T_SHOTGUN '{' 
	{
	  state.new_shotgun (New unwrap_shotgun_t ());
	}
	shotgun_calls passthrough '}'
	{
	  $$ = state.shotgun ();
	}
	;

shotgun_calls: /* empty */		
	| shotgun_calls shotgun_call	
	;

shotgun_call: passthrough callback passthrough ';' 
	{
	  parse_state_t *sg = state.shotgun ();
	  sg->passthrough ($1);
	  sg->push ($2);
	  sg->passthrough ($3);
	  sg->passthrough (";");
	}
	;

callback: '@' '(' callback_param_list_opt ')'
	{
	  $$ = New unwrap_callback_t ($3);
	}
	;

callback_param_list_opt: /* empty */ 	{ $$ = New refcounted<vartab_t> (); }
	| callback_param_list
	;

callback_param_list: callback_param
	{
	  $$ = New refcounted<vartab_t> ($1);
	}
	| callback_param_list ',' callback_param
	{
	  $1->add ($3);
	  $$ = $1;
	}
	;

identifier: T_ID
	;

callback_param: identifier             { $$ = var_t ($1); }
	| callback_stack_var
	| callback_class_var
	;

callback_stack_var: '$' '(' parameter_declaration ')'
	{
	  if (!state.stack_vars ()->add ($3)) {
	    strbuf b;
	    b << "redefinition of stack variable: " << $3.name () << "\n";
	    yyerror (b);
	  }
	  $$ = $3;
	}
	;

callback_class_var: '%' '(' parameter_declaration ')'    
	{ 
	   $3.set_as_class_var ();
	   $$ = $3; 
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
	  $$ = var_t ($1, $2);
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
init_declarator: declarator
	{
	  vartab_t *t = state.stack_vars ();
	  var_t v (state.decl_specifier (), $1);
	  if (!t->add (v)) {
	    strbuf b;
	    b << "redefinition of stack variable: " << $1->name () << "\n";
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

type_qualifier:	T_CONST		{ $$ = "const"; }
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
