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
  #include <atalk/spotlight.h>

  #include "sparql_map.h"

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
  static char sparqlvar;
  static char *result_limit;
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

%expect 5
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
    if (ssp_slq->slq_result_limit)
        result_limit = talloc_asprintf(ssp_slq, "LIMIT %ld", ssp_slq->slq_result_limit);
    else
        result_limit = "";
    ssp_result = talloc_asprintf(ssp_slq,
                                 "SELECT ?url WHERE "
                                 "{ %s . ?obj nie:url ?url . FILTER(tracker:uri-is-descendant('file://%s/', ?url)) } %s",
                                 $1, ssp_slq->slq_scope, result_limit);
    $$ = ssp_result;
}
;

expr:
BOOL {
	/*
	 * We can't properly handle these in expressions, fortunately this
	 * is probably only ever used by OS X as sole element in an
	 * expression ie "False" (when Finder window selected our share
	 * but no search string entered yet). Packet traces showed that OS
	 * X Spotlight server then returns a failure (ie -1) which is what
	 * we do here too by calling YYABORT.
	 */
	YYABORT;
}
| match OR match                 {
    if ($1 == NULL || $3 == NULL)
        YYABORT;
    if (strcmp($1, $3) != 0)
        $$ = talloc_asprintf(ssp_slq, "{ %s } UNION { %s }", $1, $3);
    else
        $$ = talloc_asprintf(ssp_slq, "%s", $1);
}
| match                        {$$ = $1; if ($$ == NULL) YYABORT;}
| function                     {$$ = $1;}
| OBRACE expr CBRACE           {$$ = talloc_asprintf(ssp_slq, "%s", $2);}
| expr AND expr                {
    if (!ssp_slq->slq_allow_expr) {
        yyerror("Spotlight queries with logic expressions are disabled");
        YYABORT;
    }
    $$ = talloc_asprintf(ssp_slq, "%s . %s", $1, $3);
}
| expr OR expr                 {
    if (!ssp_slq->slq_allow_expr) {
        yyerror("Spotlight queries with logic expressions are disabled");
        YYABORT;
    }
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
| WORD EQUAL QUOTE WORD QUOTE WORD    {$$ = map_expr($1, '=', $4);}
| WORD UNEQUAL QUOTE WORD QUOTE WORD {$$ = map_expr($1, '!', $4);}
| WORD LT QUOTE WORD QUOTE WORD     {$$ = map_expr($1, '<', $4);}
| WORD GT QUOTE WORD QUOTE WORD     {$$ = map_expr($1, '>', $4);}
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

static const char *map_daterange(const char *dateattr, time_t date1, time_t date2)
{
    EC_INIT;
    char *result = NULL;
    struct spotlight_sparql_map *p;
    struct tm *tmp;
    char buf1[64], buf2[64];

    EC_NULL_LOG( tmp = localtime(&date1) );
    strftime(buf1, sizeof(buf1), "%Y-%m-%dT%H:%M:%SZ", tmp);
    EC_NULL_LOG( tmp = localtime(&date2) );
    strftime(buf2, sizeof(buf2), "%Y-%m-%dT%H:%M:%SZ", tmp);

    for (p = spotlight_sparql_map; p->ssm_spotlight_attr; p++) {
        if (strcmp(dateattr, p->ssm_spotlight_attr) == 0) {
            result = talloc_asprintf(ssp_slq,
                                     "?obj %s ?%c FILTER (?%c > '%s' && ?%c < '%s')",
                                     p->ssm_sparql_attr,
                                     sparqlvar,
                                     sparqlvar,
                                     buf1,
                                     sparqlvar,
                                     buf2);
            sparqlvar++;
            break;
        }
    }

EC_CLEANUP:
    if (ret != 0)
        return NULL;
    return result;
}

static char *map_type_search(const char *attr, char op, const char *val)
{
    char *result = NULL;
    const char *sparqlAttr;

    for (struct MDTypeMap *p = MDTypeMap; p->mdtm_value; p++) {
        if (strcmp(p->mdtm_value, val) == 0) {
            switch (p->mdtm_type) {
            case kMDTypeMapRDF:
                sparqlAttr = "rdf:type";
                break;
            case kMDTypeMapMime:
                sparqlAttr = "nie:mimeType";
                break;
            default:
                return NULL;
            }
            result = talloc_asprintf(ssp_slq, "?obj %s '%s'",
                                     sparqlAttr,
                                     p->mdtm_sparql);
            break;
        }
    }
    return result;
}

static const char *map_expr(const char *attr, char op, const char *val)
{
    EC_INIT;
    char *result = NULL;
    struct spotlight_sparql_map *p;
    time_t t;
    struct tm *tmp;
    char buf1[64];
    bstring q = NULL, search = NULL, replace = NULL;

    for (p = spotlight_sparql_map; p->ssm_spotlight_attr; p++) {
        if (p->ssm_enabled && (strcmp(p->ssm_spotlight_attr, attr) == 0)) {
            if (p->ssm_type != ssmt_type && p->ssm_sparql_attr == NULL) {
                yyerror("unsupported Spotlight attribute");
                EC_FAIL;
            }
            switch (p->ssm_type) {
            case ssmt_bool:
                result = talloc_asprintf(ssp_slq, "?obj %s '%s'", p->ssm_sparql_attr, val);
                break;
            case ssmt_num:
                result = talloc_asprintf(ssp_slq, "?obj %s ?%c FILTER(?%c %c%c '%s')",
                                         p->ssm_sparql_attr,
                                         sparqlvar,
                                         sparqlvar,
                                         op,
                                         op == '!' ? '=' : ' ', /* append '=' to '!' */
                                         val);
                sparqlvar++;
                break;
            case ssmt_str:
                q = bformat("^%s$", val);
                search = bfromcstr("*");
                replace = bfromcstr(".*");
                bfindreplace(q, search, replace, 0);
                result = talloc_asprintf(ssp_slq, "?obj %s ?%c FILTER(regex(?%c, '%s'))",
                                         p->ssm_sparql_attr,
                                         sparqlvar,
                                         sparqlvar,
                                         bdata(q));
                sparqlvar++;
                break;
            case ssmt_fts:
                result = talloc_asprintf(ssp_slq, "?obj %s '%s'", p->ssm_sparql_attr, val);
                break;
            case ssmt_date:
                t = atoi(val) + SPRAW_TIME_OFFSET;
                EC_NULL( tmp = localtime(&t) );
                strftime(buf1, sizeof(buf1), "%Y-%m-%dT%H:%M:%SZ", tmp);
                result = talloc_asprintf(ssp_slq, "?obj %s ?%c FILTER(?%c %c '%s')",
                                         p->ssm_sparql_attr,
                                         sparqlvar,
                                         sparqlvar,
                                         op,
                                         buf1);
                sparqlvar++;
                break;
            case ssmt_type:
                result = map_type_search(attr, op, val);
                break;
            default:
                EC_FAIL;
            }
            break;
        }
    }

EC_CLEANUP:
    if (ret != 0)
        result = NULL;
    if (q)
        bdestroy(q);
    if (search)
        bdestroy(search);
    if (replace)
        bdestroy(replace);
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
    sparqlvar = 'a';

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
    ssp_slq->slq_scope = talloc_strdup(ssp_slq, "/Volumes/test");
    ssp_slq->slq_allow_expr = true;
    sparqlvar = 'a';

    s = yy_scan_string(argv[1]);

    ret = yyparse();

    yy_delete_buffer(s);

    if (ret == 0)
        printf("SPARQL: %s\n", ssp_result ? ssp_result : "(empty)");

    return 0;
} 
#endif
