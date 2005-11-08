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


%x UNWRAP TEMPLATE

%%

UNWRAP_VARS 	{ yy_push_state (UNWRAP); return T_STACKVARS; }
UNWRAP_SHOTGUN 	{ yy_push_state (UNWRAP); return T_SHOTGUN; }
UNWRAP_MEMBER_FN { yy_push_state (UNWRAP_FN); return T_MEMBER_FN; }

<DOPARSE>{
const		return T_CONST;
struct		return T_STRUCT;
extern		return T_EXTERN;
register	return T_REGISTER;
typename	return T_TYPENAME;
[{}<>;(),:*]	{ return yytext[0]; }
}

<UNWRAP_FN>{
[():]

}

<UNWRAP>{
\n		++lineno;
{WSPACE}+	/* discard */;
[{;(),_]	{ return yytext[0]; }
[}]		{ yy_pop_state (); return yytext[0]; }
}

<UNWRAP,UNWRAP_FN>{
ID		{ yylval.str = ytext; return T_ID; }
"<"		{ yy_push_state (TEMPLATE); return yytext[0]; }
}

<TEMPLATE>{
"<"		{ yy_push_state (TEMPLATE); return yytext[0]; }
\n		++lineno;
{WSPACE}+	/* discard */;
ID 		{ yylval.str = yytext; return T_ID; }
NUM 		{ yylval.str = yytext; return T_NUM; }
[,:]		{ return yytext[0]; }
">"		{ yy_pop_state (); return yytext[0]; }
}

\n		++lineno;
[^U\n]+		{ yylval.str = yytext; return T_PASSTHROUGH; }
U		{ yylval.str = yytext; return T_PASSTHROUGH; }

%%

void
nlcount (int m)
{
  int n = 0;
  for (char *y = yytext; *y; y++)
    if (*y == '\n') {
      n++;
      if (m && m == n) 
        break;
    }
   lineno++;
}


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
