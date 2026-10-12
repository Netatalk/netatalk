%top{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
}

%{
#include <stdbool.h>

#include <gio/gio.h>
#include <talloc.h>

#include <atalk/spotlight.h>

#include "sparql_parser.h"

#define SLQ_VAR ssp_slq
%}

/*!
 * @file
 * @brief Lexer of the Spotlight raw query language
 */

ASC     [a-zA-Z0-9_\*\:\-\.]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

/*!
 * @brief Split a Spotlight raw query string into tokens for the SPARQL parser
 *
 * A word is a run of ASCII letters, digits, and the characters _ * : - .,
 * or of UTF-8 multibyte sequences. Quotes, braces, the comparison and logic
 * operators, InRange, and $time.iso are separate tokens. Spaces, tabs, and
 * newlines separate tokens.
 *
 * @returns the next token, or 0 at the end of the string
 */
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
