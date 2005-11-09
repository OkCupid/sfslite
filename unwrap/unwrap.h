
#ifndef _UNWRAP_H
#define _UNWRAP_H

#include "amisc.h"
#include "vec.h"
#include "union.h"
#include "qhash.h"

struct YYSTYPE {
  ::str str;
  char ch;
};

extern YYSTYPE yylval;

extern int yylex ();
extern int yyparse ();
#undef yyerror
extern int yyerror (str);


#endif /* _UNWRAP_H */
