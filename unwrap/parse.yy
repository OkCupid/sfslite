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
%token <str> T_TEMPLATE
%token <str> T_PASSTHROUGH

/* Tokens for C++ Variable Modifiers */
%token T_CONST
%token T_STRUCT
%token T_EXTERN
%token T_REGISTER
%token T_TYPENAME

/* Keywords for our new filter */
%token T_VARS
%token T_SHOTGUN

%token T_2COLON

%%
file: /* empty * /
	| file section
	;

section:  passthrough
	| unwrap
	;

passhthrough: /* empty */
	| passthrough T_PASSTHROUGH
	;

unwrap:   unwrap_vars
	| unwrap_shotgun
	;

unwrap_vars: T_VARS '{' var_decl_list '}'
	;

unwrap_shotgun: T_SHOTGUN '{' shotgun_round '}'
	;

shotgun_round: /* empty */
	| shotgun_round shotgun_pellet
	;

shotgun_pellet: fn_call ';'
	| fn_ret_expr '=' fn_call ';'
	;

fn_ret_expr: '(' fn_ret_list ')'
	;

fn_ret_list: /* empty */
	| fn_ret_list ',' T_ID
	;

fn


var_decl_list:	/* empty * /
	| var_decl_list var_decl_line
	;

var_decl_line: c_type var_name_list ';'
	;

var_name_list: var_name
	| var_name_list ',' var_name
	;

pointers:	/* empty */
	| pointers '*'
	;

type_qualifier:	T_CONST
	| T_VOLATILE
	;

type_qualifier_list: type_qualifier
	| type_qualifier_list type_qualifier
	;

pointer: '*' type_qualifier_list
	| '*' type_qualifier_list pointer
	;

var_name: pointer T_ID 
	| T_ID
	;

c_type_with_pointer: c_type
	| c_type pointer
	;

c_type: c_type_base
	| c_type_base templated_type
	;

templated_type: '<' templated_type_list '>'
	;

templated_type_list: /* empty */
	| templated_type_list ',' templated_type_arg
	;

templated_type_arg: c_type_with_pointer
	| T_NUM
	;

c_type_base: type_specifier_list c_type_base_unspecified
	;

c_type_base_unspecified: T_ID
	| c_type_base_unspecified T_2COLON T_ID
	;

type_specifier: T_CONST
	| T_STRUCT
	| T_UNSIGNED
	| T_STATIC
	;

type_specifier_list: /* empty */
	| type_specifier_list type_specifier
	;

%%
