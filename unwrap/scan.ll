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
#define YYSTYPE YYSTYPE
#include "unwrap.h"
#include "parse.h"

#define YY_NO_UNPUT
#define YY_SKIP_YYWRAP
#define yywrap() 1

str filename = "(stdin)";
int lineno;
%}

%option stack
%option noyywrap

NUM	[-]?[0-9]+
ID	[a-zA-Z_][a-zA-Z_0-9]*
WSPACE	[ \t]
SYM	[{}<>;,():*\[\]]


%x UNWRAP UNWRAP_ENTER BLOCK

%%


<UNWRAP>{
\n		++lineno;
{WSPACE}+	/* discard */;

const		return T_CONST;
struct		return T_STRUCT;
extern		return T_EXTERN;
register	return T_REGISTER;
typename	return T_TYPENAME;
unsigned	return T_UNSIGNED;

[{]		{ yy_push_state (UNWRAP); return yytext[0]; }
[}]		{ yy_pop_state (); return yytext[0]; }

[{}<>;(),:*]	{ return yytext[0]; }

ID 		{ yylval.str = yytext; return T_ID; }

[+-]?[0-9]+	|
[+-]?0x[0-9a-fA-F]+	{ yylval.str = yytext; return T_NUM; }

}


<UNWRAP_ENTER>{
\n		++lineno;
{WSPACE}++	/* discard */ ;
[{]		{ yy_push_state (UNWRAP); return yytext[0]; }
}

<INITIAL,BLOCK>{
\n		{ yylval.str = yytext; ++lineno; return T_PASSTHROUGH; }
[^U{\n]+	{ yylval.str = yytext; return T_PASSTHROUGH; }
U		{ yylval.str = yytext; return T_PASSTHROUGH; }
[{]		{ yylval.str = yytext; yy_push_state (BLOCK); 
		  return T_PASSTHROUGH; }
[}]		{ yylval.str = yytext; yy_pop_state ();
	    	  return T_PASSTHROUGH; }

UNWRAP_VARS	{ yy_push_state (UNWRAP_ENTER); return T_VARS; }
UNWRAP_SHOTGUN	{ yy_push_state (UNWRAP_ENTER); return T_SHOTGUN; }

}

%%

int
yyerror (str msg)
{
  warnx << filename << ":" << lineno << ": " << msg << "\n";
  exit (1);
}

int
yywarn (str msg)
{
  warnx << filename << ":" << lineno << ": Warning: " << msg << "\n";
  return 0;
}
