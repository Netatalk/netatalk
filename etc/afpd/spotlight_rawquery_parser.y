%{
  #include <atalk/standards.h>

  #include <stdbool.h>
  #include <stdio.h>
  #include <string.h>
  #include <time.h>

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

  /* forward declarations */
  static const char *map_expr(const char *attr, char op, const char *val);
  static const char *map_daterange(const char *dateattr, time_t date1, time_t date2);
  static time_t isodate2unix(const char *s);
 
 /* global vars, eg needed by the lexer */
  slq_t *ssp_slq;

  /* local vars */
  static gchar *ssp_result;

%}

%code provides {
  #define SPRAW_TIME_OFFSET 978307200
  extern int map_spotlight_to_sparql_query(slq_t *slq, gchar **sparql_result);
  extern slq_t *ssp_slq;
}

%union {
    int ival;
    const char *sval;
    bool bval;
    time_t tval;
}

%expect 1
%error-verbose

%type <sval> match expr line function
%type <tval> date

%token <sval> WORD
%token <bval> BOOL
%token FUNC_INRANGE
%token DATE_ISO
%token OBRACE CBRACE EQUAL UNEQUAL GT LT COMMA QUOTE
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
BOOL                             {
    if ($1 == false)
        YYACCEPT;
    else
        YYABORT;
}
| match OR match                 {
    if (strcmp($1, $3) != 0)
        $$ = talloc_asprintf(ssp_slq, "{ %s } UNION { %s }", $1, $3);
    else
        $$ = talloc_asprintf(ssp_slq, "%s", $1);
}
| match                        {$$ = $1; if ($$ == NULL) YYABORT;}
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
WORD EQUAL QUOTE WORD QUOTE     {$$ = map_expr($1, '=', $4);}
| WORD UNEQUAL QUOTE WORD QUOTE {$$ = map_expr($1, '!', $4);}
| WORD LT QUOTE WORD QUOTE      {$$ = map_expr($1, '<', $4);}
| WORD GT QUOTE WORD QUOTE      {$$ = map_expr($1, '>', $4);}
;

function:
FUNC_INRANGE OBRACE WORD COMMA date COMMA date CBRACE {$$ = map_daterange($3, $5, $7);}
;

date:
DATE_ISO OBRACE WORD CBRACE    {$$ = isodate2unix($3);}
| WORD                         {$$ = atoi($1) + SPRAW_TIME_OFFSET;}
;

%%

static time_t isodate2unix(const char *s)
{
    struct tm tm;

    if (strptime(s, "%Y-%m-%dT%H:%M:%SZ", &tm) == NULL)
        return (time_t)-1;
    return mktime(&tm);
}

const char *map_daterange(const char *dateattr, time_t date1, time_t date2)
{
    EC_INIT;
    char *result = NULL;
    struct spotlight_sparql_map *p;
    struct tm *tmp;
    char buf1[64], buf2[64];

    EC_NULL( tmp = localtime(&date1) );
    strftime(buf1, sizeof(buf1), "%Y-%m-%dT%H:%M:%SZ", tmp);
    EC_NULL( tmp = localtime(&date2) );
    strftime(buf2, sizeof(buf2), "%Y-%m-%dT%H:%M:%SZ", tmp);

    for (p = spotlight_sparql_map; p->ssm_spotlight_attr; p++) {
        if (strcmp(dateattr, p->ssm_spotlight_attr) == 0) {
            result = talloc_asprintf(ssp_slq,
                                     "?x %s ?d FILTER (?d > '%s' && ?d < '%s')",
                                     p->ssm_sparql_attr,
                                     buf1,
                                     buf2);
        }
    }

EC_CLEANUP:
    if (ret != 0)
        return NULL;
    return result;
}

const char *map_expr(const char *attr, char op, const char *val)
{
    EC_INIT;
    char *result = NULL;
    struct spotlight_sparql_map *p;
    time_t t;
    struct tm *tmp;
    char buf1[64];

    for (p = spotlight_sparql_map; p->ssm_spotlight_attr; p++) {
        if (strcmp(p->ssm_spotlight_attr, attr) == 0) {
            switch (p->ssm_type) {
            case ssmt_bool:
                result = talloc_asprintf(ssp_slq, "?x %s '%s'", p->ssm_sparql_attr, val);
                break;
            case ssmt_num:
                result = talloc_asprintf(ssp_slq, "?x %s ?y FILTER(?y %c '%s')", p->ssm_sparql_attr, op, val);
                break;
            case ssmt_str:
                result = talloc_asprintf(ssp_slq, "?x %s ?y FILTER(regex(?y, '%s'))", p->ssm_sparql_attr, val);
                break;
            case ssmt_fts:
                result = talloc_asprintf(ssp_slq, "?x %s '%s'", p->ssm_sparql_attr, val);
                break;
            case ssmt_date:
                t = atoi(val) + SPRAW_TIME_OFFSET;
                EC_NULL( tmp = localtime(&t) );
                strftime(buf1, sizeof(buf1), "%Y-%m-%dT%H:%M:%SZ", tmp);
                result = talloc_asprintf(ssp_slq, "?x %s ?y FILTER(?y %c '%s')", p->ssm_sparql_attr, op, buf1);
                break;
            default:
                yyerror("unknown Spotlight attribute type");
                EC_FAIL;
            }
            break;
        }
    }

EC_CLEANUP:
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

/**
 * Map a Spotlight RAW query string to a SPARQL query string
 *
 * @param[in]     slq            Spotlight query handle
 * @param[out]    sparql_result  Mapped SPARQL query, string is allocated in
 *                               talloc context of slq
 * @return        0 on success, -1 on error
 **/
int map_spotlight_to_sparql_query(slq_t *slq, gchar **sparql_result)
{
    EC_INIT;
    YY_BUFFER_STATE s = NULL;
    ssp_result = NULL;

    ssp_slq = slq;
    s = yy_scan_string(slq->slq_qstring);

    EC_ZERO( yyparse() );

EC_CLEANUP:
    if (s)
        yy_delete_buffer(s);
    if (ret == 0)
        *sparql_result = ssp_result;
    else
        *sparql_result = NULL;
    EC_EXIT;
}

#ifdef MAIN
int main(int argc, char **argv)
{
    int ret;
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

    ret = yyparse();

    yy_delete_buffer(s);

    if (ret == 0)
        printf("SPARQL: %s\n", ssp_result ? ssp_result : "(empty)");

    return 0;
} 
#endif
