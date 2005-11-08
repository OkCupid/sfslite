
#ifndef _UNWRAP_H
#define _UNWRAP_H

struct yystype {
  ::str str;
  char ch;
};

#define YYSTYPE yystype
extern YYSTYPE yylval;

#endif /* _UNWRAP_H */
