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
int lineno = 1;
static void switch_to_state (int i);
static int std_ret (int i);
int get_yy_lineno () { return lineno ;}
%}

%option stack

ID	[a-zA-Z_][a-zA-Z_0-9]*
WSPACE	[ \t]
SYM	[{}<>;,():*\[\]]
DNUM 	[+-]?[0-9]+
XNUM 	[+-]?0x[0-9a-fA-F]

%x FULL_PARSE FN_ENTER VARS_ENTER SHOTGUN_ENTER SHOTGUN_CB_ENTER SHOTGUN
%x UNWRAP SHOTGUN_CB PAREN_ENTER UNWRAP_BASE C_COMMENT C_COMMENT_GOBBLE
%x EXPR EXPR_BASE ID_OR_NUM NUM_ONLY SHOTGUN_BASE HALF_PARSE PP PP_BASE

%%

<FN_ENTER,FULL_PARSE,SHOTGUN_ENTER,SHOTGUN_CB_ENTER,PAREN_ENTER,SHOTGUN_CB,VARS_ENTER,ID_OR_NUM,NUM_ONLY,HALF_PARSE>{
\n		++lineno;
{WSPACE}+	/*discard*/;
}

<ID_OR_NUM>{
{ID} 		{ yy_pop_state (); return std_ret (T_ID); }
{DNUM}|{XNUM}	{ yy_pop_state (); return std_ret (T_NUM); }
.		{ return yyerror ("expected an identifier or a number"); }
}

<NUM_ONLY>
{
{DNUM}|{XNUM}	{ yy_pop_state (); return std_ret (T_NUM); }
.		{ return yyerror ("expected a number"); }
}

<FULL_PARSE,HALF_PARSE>{

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

{ID} 		{ return std_ret (T_ID); }
{DNUM}|{XNUM}	{ return std_ret (T_NUM); }

[{]		{ yy_push_state (FULL_PARSE); return yytext[0]; }
[})]		{ yy_pop_state (); return yytext[0]; }

[<>;,:*]	{ return yytext[0]; }
"::"		{ return T_2COLON; }
}

<HALF_PARSE>{
[(]		{ yy_push_state (PP_BASE); return yytext[0]; }
}

<PP_BASE>{
[)]		{ yy_pop_state (); return yytext[0]; } 
}

<PP,PP_BASE>{
\n		{ ++lineno; }
[^()\n]+	{ return std_ret (T_PASSTHROUGH); }
[(]		{ yy_push_state (PP); return std_ret (T_PASSTHROUGH); }
}

<PP>{
[)]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<FULL_PARSE>{
[(]		{ yy_push_state (FULL_PARSE); return yytext[0]; }
}

<HALF_PARSE,FULL_PARSE>{
.		{ return yyerror ("illegal token found in parsed "
				  "environment"); }
}


<FN_ENTER>{
[(]		{ yy_push_state (FULL_PARSE); return yytext[0]; }
[{]		{ switch_to_state (UNWRAP_BASE); return yytext[0]; }
.		{ return yyerror ("illegal token found in function "
				  "environment"); }
}

<VARS_ENTER>{
[{]		{ switch_to_state (HALF_PARSE); return yytext[0]; }
.		{ return yyerror ("illegal token found between VARS and '{'");}
}

<SHOTGUN_ENTER>{
[{]		{ switch_to_state (SHOTGUN_BASE); return yytext[0]; }
.		{ return yyerror ("illegal token found between SHOTGUN "
				  "and '{'");}
}

<SHOTGUN_CB_ENTER>{
[(\[]		{ switch_to_state (SHOTGUN_CB); return yytext[0]; }
.		{ return yyerror ("illegal token found between '@' and '('"); }
}

<PAREN_ENTER>{
[(]		{ switch_to_state (FULL_PARSE); return yytext[0]; }
.		{ return yyerror ("illegal token found in $(..) or %(..)"); }
}

<SHOTGUN_CB>{
"("		{ yy_push_state (EXPR_BASE); 
	          yy_push_state (FULL_PARSE); /* will return the ')' */
 	  	  return yytext[0]; }
{ID}		{ return std_ret (T_ID); }
")"		{ yy_pop_state (); return yytext[0]; }
[;,]		{ return yytext[0]; }
.		{ return yyerror ("illegal token found in '@(..)'"); }
}

<SHOTGUN_BASE,SHOTGUN>{
\n		{ ++lineno; return std_ret (T_PASSTHROUGH); }
[^@{}\n/]+	{ return std_ret (T_PASSTHROUGH); }
@		{ yy_push_state (SHOTGUN_CB_ENTER); return yytext[0]; }
[{]		{ yy_push_state (SHOTGUN); return std_ret (T_PASSTHROUGH); }
}

<SHOTGUN_BASE>{
[}]		{ yy_pop_state (); return yytext[0]; }
}

<SHOTGUN>{
[}]		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
}

<SHOTGUN_BASE,SHOTGUN>{
.		{ return yyerror ("illegal token found in SHOTGUN { ... } "); }
}

<EXPR_BASE>{
\n		++lineno;
[,;]		{ yy_pop_state (); return yytext[0]; }
[)]		{ 
	 	  /* 
	           * XXX HACK; pop out of EXPR_BASE and also SHOTGUN_CB!!
		   */
	          yy_pop_state (); yy_pop_state (); return yytext[0]; 
	        }
}

<EXPR,EXPR_BASE>{
[(]		{ yylval.str = yytext; yy_push_state (EXPR); 
                  return T_PASSTHROUGH; }
[^()\n/$@%,;]+	{ yylval.str = yytext; return T_PASSTHROUGH; }
"%%"		{ return T_2PCT; }
[$]		{ return yytext[0]; }
[@]		{ yy_push_state (NUM_ONLY); return yytext[0]; }
[%]		{ yy_push_state (ID_OR_NUM); return yytext[0]; }
"$$"		{ yy_push_state (ID_OR_NUM); return T_2DOLLAR; }
}

<EXPR>{
,		{ return std_ret (T_PASSTHROUGH); }
")"		{ yy_pop_state (); return std_ret (T_PASSTHROUGH); }
;		{ return yyerror ("stray ';' found in expression"); }
.		{ return yyerror ("illegal token found in @(...)"); }
}

<EXPR_BASE>{
.		{ return yyerror ("unbalanced paranthesis"); }
}

<UNWRAP_BASE,UNWRAP>{
\n		{ yylval.str = yytext; ++lineno; return T_PASSTHROUGH; }
[^VSC{}\n/]+|[VSC] { yylval.str = yytext; return T_PASSTHROUGH; }
[{]		{ yylval.str = yytext; yy_push_state (UNWRAP); 
		  return T_PASSTHROUGH; }

VARS		{ yy_push_state (VARS_ENTER); return T_VARS; }
SHOTGUN|CRCC	{ yy_push_state (SHOTGUN_ENTER); return T_SHOTGUN; }
}

<UNWRAP>{
[}]		{ yylval.str = yytext; yy_pop_state ();
	    	  return T_PASSTHROUGH; }
}

<UNWRAP_BASE>{
[}]		{ return yytext[0]; }
}

[^UF\n]+|[UF]	{ yylval.str = yytext; return T_PASSTHROUGH ; }
\n		{ ++lineno; yylval.str = yytext; return T_PASSTHROUGH; }

UNWRAP|FUNCTION { yy_push_state (FN_ENTER); return T_UNWRAP; }
UNWRAP_SD	{ yy_push_state (FN_ENTER); return T_UNWRAP_SD; }


<UNWRAP,UNWRAP_BASE>{
"//"[^\n]*\n	{ ++lineno ; yylval.str = yytext; return T_PASSTHROUGH; }
"//"[^\n]*	{ yylval.str = yytext; return T_PASSTHROUGH; }
"/*"		{ yy_push_state (C_COMMENT); yylval.str = yytext;
	          return T_PASSTHROUGH; }
}

<C_COMMENT>{
"*/"		{ yy_pop_state (); yylval.str = yytext; return T_PASSTHROUGH; }
"*"		{ yylval.str = yytext; return T_PASSTHROUGH ; }
[^*\n]*		{ yylval.str = yytext; return T_PASSTHROUGH ; }
\n		{ ++lineno; yylval.str = yytext; return T_PASSTHROUGH; }
}


<SHOTGUN_CB_ENTER,PAREN_ENTER,FULL_PARSE,SHOTGUN_CB,SHOTGUN,FN_ENTER,VARS_ENTER,EXPR,EXPR_BASE,HALF_PARSE,PP,PP_BASE>{

"//"[^\n]*\n	++lineno ;
"//"[^\n]*	/* discard */ ;
"/*"		{ yy_push_state (C_COMMENT_GOBBLE); }

}

<C_COMMENT_GOBBLE>{
"*/"		{ yy_pop_state (); }
"*"		/* ignore */ ;
[^*\n]*		/* ignore */ ;
\n		++lineno; 
}

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

int
std_ret (int i)
{
  yylval.str = yytext;
  return i;
}
