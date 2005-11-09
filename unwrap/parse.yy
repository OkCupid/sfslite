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

%%

file:  passthrough
	| file fn passthrough
	;

passthrough: /* empty */
	| passthrough T_PASSTHROUGH
	;

fn:	T_FUNCTION '(' fn_declaration ')' fn_body 
	;

fn_body: '{' fn_statements '}'
	;

/* declaration_specifiers is no longer optional ?! */
fn_declaration: declaration_specifiers fancy_declarator const_opt
	;

const_opt: /* empty */
	| T_CONST
	;

fn_statements:  /* empty */
	| fn_statements fn_statement 
	;

fn_statement: passthrough
	| vars
	| shotgun
	;

vars:	T_VARS '{' declaration_list_opt '}'
	;

shotgun: T_SHOTGUN '{' shotgun_calls '}'
	;

shotgun_calls: /* empty */
	| shotgun_calls shotgun_call
	;

shotgun_call: passthrough callback passthrough ';'
	;

callback: '@' '(' callback_param_list ')'
	;

callback_param_list: /* empty */
	| callback_param_list ',' callback_param
	;

identifier: T_ID
	;

callback_param: identifier
	| callback_stack_var
	| callback_class_var
	;

callback_stack_var: '$' '(' parameter_declaration ')'
	;

callback_class_var: '%' '(' parameter_declaration ')'
	;

declaration_list_opt: /* empty */
	| declaration_list
	;

declaration_list: declaration
	| declaration_list declaration
	;

/* missing: '...'
 */
parameter_type_list: parameter_list
	;

parameter_list: /* empty */
	| parameter_list ',' parameter_declaration
	;

/* missing: abstract declarators
 */
parameter_declaration: declaration_specifiers declarator
	;

declaration: declaration_specifiers init_declarator_list_opt ';'
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
	;

declarator: pointer_opt direct_declarator
	;

fancy_declarator: pointer fancy_direct_declarator
	| fancy_direct_declarator
	;

/* accommodate my_class_t::foo
 */
fancy_direct_declarator: typedef_name
	| fancy_direct_declarator '(' parameter_type_list ')'
	;

/* missing: arrays, and C++-style initialization
 */
direct_declarator: identifier
	| direct_declarator '(' parameter_type_list ')'
	;

declaration_specifiers_opt: /* empty */
	| declaration_specifiers
	;

/* missing: first rule:
 *	storage_class_specifier declaration_specifiers_opt
 */
declaration_specifiers: 
	| type_specifier declaration_specifiers_opt
	| type_qualifier declaration_specifiers_opt
	;

/* missing: struct and enum rules:
 *	| struct_or_union_specifier
 *	| enum_specifier
 */
type_specifier: T_VOID
	| T_CHAR
	| T_SHORT
	| T_INT
	| T_LONG
	| T_FLOAT
	| T_DOUBLE
	| T_SIGNED
	| T_UNSIGNED
	| typedef_name
	;

type_qualifier:	T_CONST
	;

type_qualifier_list: type_qualifier
	| type_qualifier_list type_qualifier
	;

type_qualifier_list_opt: /* empty */
	| type_qualifier_list
	;

/*
 * foo<int, char *>::bar_t::my_class<int> -> 
 *   foo<> bar_t my_class<>
 */
typedef_name:  typedef_name_single
	| typedef_name T_2COLON typedef_name_single
	;

typedef_name_single: identifier template_instantiation_opt
	;

template_instantiation_opt: /* empty */
	| template_instantiation
	;

template_instantiation: '<' template_instantiation_list '>'
	;

template_instantiation_list: /* empty */
	| template_instantiation_list ',' template_instantiation_arg
	;

template_instantiation_arg: declaration_specifiers pointer_opt
	;

pointer_opt: /* empty */
	| pointer
	;

pointer: '*'
	| '*' type_qualifier_list_opt pointer
	;

%%
