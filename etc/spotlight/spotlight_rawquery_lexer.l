%top{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
}

%{
#include <stdbool.h>
#include <gio/gio.h>
#include <atalk/talloc.h>
#include <atalk/spotlight.h>
#ifdef HAVE_TRACKER
#include "sparql_parser.h"
#define SLQ_VAR ssp_slq
#endif
%}

ASC     [a-zA-Z0-9_\*\:\-\.]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

%%
InRange                               return FUNC_INRANGE;
\$time\.iso                           return DATE_ISO;
false                                 {yylval.bval = false; return BOOL;}
true                                  {yylval.bval = true; return BOOL;}
\"                                    return QUOTE;
\(                                    return OBRACE;
\)                                    return CBRACE;
\&\&                                  return AND;
\|\|                                  return OR;
\=\=                                  return EQUAL;
\=                                    return EQUAL;
\!\=                                  return UNEQUAL;
\<                                    return LT;
\>                                    return GT;
\,                                    return COMMA;
{UANY}+                               {yylval.sval = talloc_strdup(SLQ_VAR, yytext); return WORD;}
[ \t\n]                               /* ignore */
%%
