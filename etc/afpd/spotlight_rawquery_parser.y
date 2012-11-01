%{
  #include <stdbool.h>
  #include <stdio.h>
  #include <string.h>
  #include <gio/gio.h>
  #include <atalk/talloc.h>
  #include <atalk/logger.h>
  #include <atalk/errchk.h>
  #include "spotlight_SPARQL_map.h"
  #include "spotlight.h"

  struct yy_buffer_state;
  typedef struct yy_buffer_state *YY_BUFFER_STATE;
  extern int yylex (void);
  extern void yyerror (char const *);
  extern void *yyterminate(void);
  extern YY_BUFFER_STATE yy_scan_string( const char *str);
  extern void yy_delete_buffer ( YY_BUFFER_STATE buffer );

  static const char *map_expr(const char *attr, const char *val);
  static const char *map_daterange(const char *dateattr, const char *date1, const char *date2);

  slq_t *ssp_slq;
  gchar *ssp_result;

%}

%code provides {
  extern const gchar *map_spotlight_to_sparql_query(slq_t *slq);
  extern slq_t *ssp_slq;
  extern gchar *ssp_result;
}

%union {
    int ival;
    const char *sval;
}

%expect 1
%error-verbose

%type <sval> match expr line function
%token <sval> DATE
%token <sval> WORD
%token FUNC_INRANGE
%token DATE_SPEC
%token OBRACE CBRACE EQUAL COMMA QUOTE
%left AND
%left OR
%%

input:
/* empty */
| input line
;
     
line:
expr                           {
    ssp_result = talloc_asprintf(ssp_slq,
                                 "SELECT DISTINCT ?url WHERE "
                                 "{ ?x nie:url ?url FILTER(fn:starts-with(?url, 'file://%s/')) . %s}",
                                 ssp_slq->slq_vol->v_path, $1);
    $$ = ssp_result;
}
;

expr:
match OR match                 {
    if (strcmp($1, $3) != 0)
        $$ = talloc_asprintf(ssp_slq, "{ %s } UNION { %s }", $1, $3);
    else
        $$ = talloc_asprintf(ssp_slq, "%s", $1);
}
| match                        {$$ = $1;}
| function                     {$$ = $1;}
| OBRACE expr CBRACE           {$$ = talloc_asprintf(ssp_slq, "%s", $2);}
| expr AND expr                {$$ = talloc_asprintf(ssp_slq, "%s . %s", $1, $3);}
| expr OR expr                 {
    if (strcmp($1, $3) != 0)
        $$ = talloc_asprintf(ssp_slq, "{ %s } UNION { %s }", $1, $3);
    else
        $$ = talloc_asprintf(ssp_slq, "%s", $1);
}
;

match:
WORD EQUAL QUOTE WORD QUOTE    {$$ = map_expr($1, $4);}
;

function:
FUNC_INRANGE OBRACE WORD COMMA DATE_SPEC OBRACE DATE CBRACE COMMA DATE_SPEC OBRACE DATE CBRACE CBRACE {$$ = map_daterange($3, $7, $12);}
;

%%

const char *map_daterange(const char *dateattr, const char *date1, const char *date2)
{
    char *result = NULL;
    struct spotlight_sparql_map *p;

    for (p = spotlight_sparql_map; p->ssm_spotlight_attr; p++) {
        if (strcmp(dateattr, p->ssm_spotlight_attr) == 0) {
            result = talloc_asprintf(ssp_slq,
                                     "?x %s ?d FILTER (?d > '%s' && ?d < '%s')",
                                     p->ssm_sparql_attr,
                                     date1,
                                     date2);
        }
    }


    return result;
}

const char *map_expr(const char *attr, const char *val)
{
    char *result = NULL;
    struct spotlight_sparql_map *p;

    for (p = spotlight_sparql_map; p->ssm_spotlight_attr; p++) {
        if (strcmp(p->ssm_spotlight_attr, attr) == 0) {
            result = talloc_asprintf(ssp_slq, p->ssm_sparql_query_fmtstr, val);
            break;
        }
    }

    return result;
}

void yyerror(const char *str)
{
#ifdef MAIN
    printf("yyerror: %s\n", str);
#else
    LOG(log_error, logtype_sl, "yyerror: %s", str);
#endif
}
 
int yywrap()
{
    return 1;
} 

const gchar *map_spotlight_to_sparql_query(slq_t *slq)
{
    EC_INIT;
    YY_BUFFER_STATE s = NULL;

    ssp_slq = slq;
    s = yy_scan_string(slq->slq_qstring);

    EC_ZERO( yyparse() );

EC_CLEANUP:
    if (s)
        yy_delete_buffer(s);

    return ssp_result;
}

#ifdef MAIN
int main(int argc, char **argv)
{
    YY_BUFFER_STATE s;

    if (argc != 2) {
        printf("usage: %s QUERY\n", argv[0]);
        return 1;
    }

    ssp_slq = talloc_zero(NULL, slq_t);
    struct vol *vol = talloc_zero(ssp_slq, struct vol);
    vol->v_path = "/Volumes/test";
    ssp_slq->slq_vol = vol;

    s = yy_scan_string(argv[1]);

    yyparse();

    yy_delete_buffer(s);

    printf("SPARQL: %s\n", ssp_result);

    return 0;
} 
#endif
