%{
#include <string.h>
#include <gio/gio.h>
#include <atalk/talloc.h>
#include "spotlight.h"
#include "spotlight_rawquery_parser.h"
%}

%%
InRange                               return FUNC_INRANGE;
\$time\.iso                           return DATE_SPEC;
....-..-..T..:..:..Z                  {yylval.sval = talloc_strdup(ssp_slq, yytext); return DATE;}
cwd                                   /* ignore case specifier */
[a-zA-Z\*]+                           {yylval.sval = talloc_strdup(ssp_slq, yytext); return WORD;}
\"                                    return QUOTE;
\(                                    return OBRACE;
\)                                    return CBRACE;
\&\&                                  return AND;
\|\|                                  return OR;
\=\=                                  return EQUAL;
\,                                    return COMMA;
[ \t\n]                               /* ignore */
%%
