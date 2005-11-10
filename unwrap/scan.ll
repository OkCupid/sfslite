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
static void switch_to_state (int i);
%}

%option stack
%option noyywrap

ID	[a-zA-Z_][a-zA-Z_0-9]*
WSPACE	[ \t]
SYM	[{}<>;,():*\[\]]
DNUM 	[+-]?[0-9]+
XNUM 	[+-]?0x[0-9a-fA-F]

%x FULL_PARSE FN_ENTER VARS_ENTER SHOTGUN_ENTER SHOTGUN_CB_ENTER SHOTGUN
%x UNWRAP SHOTGUN_CB PAREN_ENTER UNWRAP_BASE

%%


<FULL_PARSE>{
\n		++lineno;
{WSPACE}+	/* discard */;

const		return T_CONST;
struct		return T_STRUCT;
typename	return T_TYPENAME;
void		return T_VOID;
char		return T_CHAR;
short		return T_SHORT;
int		return T_INT;
long		return T_LONG;
float		return T_FLOAT;
double		return T_DOUBLE;
signed		return T_SIGNED;
unsigned	return T_UNSIGNED;
static		return T_STATIC;

[{(]		{ yy_push_state (FULL_PARSE); return yytext[0]; }
[})]		{ yy_pop_state (); return yytext[0]; }

[<>;,:*]	{ return yytext[0]; }
"::"		{ return T_2COLON; }

{ID} 		{ yylval.str = yytext; return T_ID; }

{DNUM}|{XNUM}	{ yylval.str = yytext; return T_NUM; }

.		{ return yyerror ("illegal token found in parsed "
				  "environment"); }

}

<FN_ENTER>{
\n		++lineno;
{WSPACE}+	/*discard*/;
[(]		{ yy_push_state (FULL_PARSE); return yytext[0]; }
[{]		{ switch_to_state (UNWRAP_BASE); return yytext[0]; }
.		{ return yyerror ("illegal token found in function "
				  "environment"); }
}

<VARS_ENTER>{
\n		++lineno;
{WSPACE}+	/* discard */ ;
[{]		{ switch_to_state (FULL_PARSE); return yytext[0]; }
.		{ return yyerror ("illegal token found between VARS and '{'");}
}

<SHOTGUN_ENTER>{
\n		++lineno;
{WSPACE}+	/* discard */ ;
[{]		{ switch_to_state (SHOTGUN); return yytext[0]; }
.		{ return yyerror ("illegal token found between SHOTGUN "
				  "and '{'");}
}

<SHOTGUN_CB_ENTER>{
\n		++lineno;
{WSPACE}+	/* discard */ ;
[(]		{ switch_to_state (SHOTGUN_CB); return yytext[0]; }
.		{ return yyerror ("illegal token found between '@' and '('"); }
}

<PAREN_ENTER>{
\n		++lineno;
{WSPACE}+	/*discard*/;
[(]		{ switch_to_state (FULL_PARSE); return yytext[0]; }
.		{ return yyerror ("illegal token found in $(..) or %(..)"); }
}

<SHOTGUN_CB>{
\n		++lineno;
{WSPACE}+	/*discard*/;
[(]		{ yy_push_state (SHOTGUN_CB); return yytext[0]; }
{ID}		{ yylval.str = yytext; return T_ID; }
[)]		{ yy_pop_state (); return yytext[0]; }
[%$]		{ yy_push_state (PAREN_ENTER); return yytext[0]; }
,		{ return yytext[0]; }
.		{ return yyerror ("illegal token found in '@(..)'"); }
}

<SHOTGUN>{
\n		{ ++lineno; yylval.str = yytext; return T_PASSTHROUGH; }
[^@{};\n]+	{ yylval.str = yytext; return T_PASSTHROUGH; }
@		{ yy_push_state (SHOTGUN_CB_ENTER); return yytext[0]; }
[;]		{ return yytext[0]; }
[}]		{ yy_pop_state (); return yytext[0]; }
.		{ return yyerror ("illegal token found in SHOTGUN { ... } "); }
}

<UNWRAP_BASE,UNWRAP>{
\n		{ yylval.str = yytext; ++lineno; return T_PASSTHROUGH; }
[^VS{}\n]+|[VS]	{ yylval.str = yytext; return T_PASSTHROUGH; }
[{]		{ yylval.str = yytext; yy_push_state (UNWRAP); 
		  return T_PASSTHROUGH; }

VARS		{ yy_push_state (VARS_ENTER); return T_VARS; }
SHOTGUN		{ yy_push_state (SHOTGUN_ENTER); return T_SHOTGUN; }
}

<UNWRAP>{
[}]		{ yylval.str = yytext; yy_pop_state ();
	    	  return T_PASSTHROUGH; }
}

/* In the base UNWRAP frame, we need to return '}' and not PASSTHROUGH
 * to the parser, since the parser needs to know when to close the
 * FUNCTION () { ... } section
 */
<UNWRAP_BASE>{
[}]		{ return yytext[0]; }
}

[^MF\n]+|[MF]	{ yylval.str = yytext; return T_PASSTHROUGH ; }
\n		{ ++lineno; yylval.str = yytext; return T_PASSTHROUGH; }

MEMBER   	{ yy_push_state (UNWRAP); 
	          yy_push_state (FN_ENTER); 
                  return T_MEMBER; 
		}

FUNCTION	{ yy_push_state (FN_ENTER); return T_FUNCTION; }

%%

void
switch_to_state (int s)
{
	yy_pop_state ();
	yy_push_state (s);
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
