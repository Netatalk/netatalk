%{
#include <string.h>
#include <stdbool.h>
#include <gio/gio.h>
#include <atalk/talloc.h>
#include "spotlight.h"
#include "spotlight_rawquery_parser.h"
%}

%%
InRange                               return FUNC_INRANGE;
\$time\.iso                           return DATE_SPEC;
....-..-..T..:..:..Z                  {yylval.sval = talloc_strdup(ssp_slq, yytext); return DATE;}
false                                 {yylval.bval = false; return BOOL;}
true                                  {yylval.bval = true; return BOOL;}
[a-zA-Z\*]+                           {yylval.sval = talloc_strdup(ssp_slq, yytext); return WORD;}
\"                                    return QUOTE;
\(                                    return OBRACE;
\)                                    return CBRACE;
\&\&                                  return AND;
\|\|                                  return OR;
\=\=                                  return EQUAL;
!\=                                   return UNEQUAL;
\<                                    return LT;
\>                                    return GT;
\,                                    return COMMA;
[ \t\n]                               /* ignore */
cwd                                   /* ignore case specifier */
cd                                    /* ignore */
%%
