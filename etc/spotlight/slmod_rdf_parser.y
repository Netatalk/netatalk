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

  #include "slmod_rdf_map.h"

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
  slq_t *srp_slq;

  /* local vars */
  static gchar *srp_result;
  static gchar *srp_fts;
%}

%code provides {
  #define SPRAW_TIME_OFFSET 978307200
  extern int map_spotlight_to_rdf_query(slq_t *slq, gchar **rdf_result, gchar **fts_result);
  extern slq_t *srp_slq;
}

%union {
    int ival;
    const char *sval;
    bool bval;
    time_t tval;
}

%expect 4
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
    srp_result = talloc_asprintf(srp_slq,
                                 "<rdfq:Condition>\n"
                                 "  <rdfq:and>\n"
                                 "    <rdfq:startsWith>\n"
                                 "      <rdfq:Property name=\"File:Path\" />\n"
                                 "      <rdf:String>%s</rdf:String>\n"
                                 "    </rdfq:startsWith>\n"
                                 "    %s\n"
                                 "  </rdfq:and>\n"
                                 "</rdfq:Condition>\n",
                                 srp_slq->slq_vol->v_path, $1);
    $$ = srp_result;
}
;

expr:
BOOL                             {
    if ($1 == false)
        YYACCEPT;
    else
        YYABORT;
}
| match                        {$$ = $1; if ($$ == NULL) YYABORT;}
| function                     {$$ = $1;}
| OBRACE expr CBRACE           {$$ = talloc_asprintf(srp_slq, "%s\n", $2);}
| expr AND expr                {$$ = talloc_asprintf(srp_slq, "<rdfq:and>\n%s\n%s\n</rdfq:and>\n", $1, $3);}
| expr OR expr                 {
    if (strcmp($1, "") == 0 || strcmp($3, "") == 0) {
        /*
         * The default Spotlight search term issued by the Finder (10.8) is:
         * '* == "searchterm" || kMDItemTextContent == "searchterm"'
         * As it isn't mappable to a single Tracker RDF query, we silently
         * map ANY FTS query expression being part of an OR compound
         * expression to a simple filename search.
         * FTS queries are thus only possible by explicitly requesting
         * file content FTS search in the Finder on the client (resulting
         * in a 'kMDItemTextContent == "searchterm"' query).
         */
        if (strcmp($1, "") == 0)
            $$ = talloc_asprintf(srp_slq, $3);
        else
            $$ = talloc_asprintf(srp_slq, $1);
        talloc_free(srp_fts);
        srp_fts = NULL;
    } else {
        $$ = talloc_asprintf(srp_slq, "<rdfq:or>\n%s\n%s\n</rdfq:or>\n", $1, $3);
    }
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
    struct spotlight_rdf_map *p;
    struct tm *tmp;
    char buf1[64], buf2[64];

    EC_NULL_LOG( tmp = localtime(&date1) );
    strftime(buf1, sizeof(buf1), "%Y-%m-%dT%H:%M:%SZ", tmp);
    EC_NULL_LOG( tmp = localtime(&date2) );
    strftime(buf2, sizeof(buf2), "%Y-%m-%dT%H:%M:%SZ", tmp);

    for (p = spotlight_rdf_map; p->srm_spotlight_attr; p++) {
        if (strcmp(dateattr, p->srm_spotlight_attr) == 0) {
                result = talloc_asprintf(srp_slq,
                                         "<rdfq:and>\n"
                                         "  <rdfq:greaterThan>\n"
                                         "    <rdfq:Property name=\"%s\" />\n"
                                         "    <rdf:Date>%s</rdf:Date>\n"
                                         "  </rdfq:greaterThan>\n"
                                         "  <rdfq:lessThan>\n"
                                         "    <rdfq:Property name=\"%s\" />\n"
                                         "    <rdf:Date>%s</rdf:Date>\n"
                                         "  </rdfq:lessThan>\n"
                                         "</rdfq:and>\n",
                                         p->srm_rdf_attr, buf1,
                                         p->srm_rdf_attr, buf2);
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

    for (struct MDTypeMap *p = MDTypeMap; p->mdtm_value; p++) {
        if (strcmp(p->mdtm_value, val) == 0) {
            if (!p->mdtm_type)
                return NULL;
            result = talloc_asprintf(srp_slq,
                                     "<rdfq:%s>\n"
                                     "  <rdfq:Property name=\"File:Mime\" />\n"
                                     "  <rdf:String>%s</rdf:String>\n"
                                     "</rdfq:%s>\n",
                                     p->mdtm_rdfop,
                                     p->mdtm_type,
                                     p->mdtm_rdfop);
            break;
        }
    }
    return result;
}

static const char *map_expr(const char *attr, char op, const char *val)
{
    EC_INIT;
    char *result = NULL;
    struct spotlight_rdf_map *p;
    time_t t;
    struct tm *tmp;
    char buf1[64];
    bstring q = NULL, search = NULL, replace = NULL;
    char *rdfop;

    for (p = spotlight_rdf_map; p->srm_spotlight_attr; p++) {
        if (p->srm_rdf_attr && strcmp(p->srm_spotlight_attr, attr) == 0) {
            switch (p->srm_type) {
            case srmt_num:
                q = bformat("^%s$", val);
                search = bfromcstr("*");
                replace = bfromcstr(".*");
                bfindreplace(q, search, replace, 0);
                result = talloc_asprintf(srp_slq,
                                         "<rdfq:regex>\n"
                                         "  <rdfq:Property name=\"%s\" />\n"
                                         "  <rdf:String>%s</rdf:String>\n"
                                         "</rdfq:regex>\n",
                                         p->srm_rdf_attr,
                                         bdata(q));
                bdestroy(q);
                break;

            case srmt_str:
                q = bformat("^%s$", val);
                search = bfromcstr("*");
                replace = bfromcstr(".*");
                bfindreplace(q, search, replace, 0);
                result = talloc_asprintf(srp_slq,
                                         "<rdfq:regex>\n"
                                         "  <rdfq:Property name=\"%s\" />\n"
                                         "  <rdf:String>%s</rdf:String>\n"
                                         "</rdfq:regex>\n",
                                         p->srm_rdf_attr,
                                         bdata(q));
                bdestroy(q);
                break;

            case srmt_fts:
                if (srp_fts) {
                    yyerror("only single fts query allowed");
                    EC_FAIL;
                }
                q = bfromcstr(val);
                search = bfromcstr("*");
                replace = bfromcstr("");
                bfindreplace(q, search, replace, 0);
                srp_fts = talloc_strdup(srp_slq, bdata(q));
                result = "";
                break;

            case srmt_date:
                t = atoi(val) + SPRAW_TIME_OFFSET;
                EC_NULL( tmp = localtime(&t) );
                strftime(buf1, sizeof(buf1), "%Y-%m-%dT%H:%M:%SZ", tmp);

                switch (op) {
                case '=':
                    rdfop = "equals";
                case '<':
                    rdfop = "lessThan";
                case '>':
                    rdfop = "greaterThan";
                default:
                    yyerror("unknown date comparison");
                    EC_FAIL;
                }
                result = talloc_asprintf(srp_slq,
                                         "<rdfq:%s>\n"
                                         "  <rdfq:Property name=\"%s\" />\n"
                                         "  <rdf:Date>%s</rdf:Date>\n"
                                         "</rdfq:%s>\n",
                                         rdfop,
                                         p->srm_rdf_attr,
                                         buf1,
                                         rdfop);

                break;

            case srmt_type:
                result = map_type_search(attr, op, val);
                break;

            default:
                yyerror("unknown Spotlight attribute type");
                EC_FAIL;
            }
            break;
        }
    }

EC_CLEANUP:
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
 * Map a Spotlight RAW query string to a RDF query
 *
 * @param[in]     slq            Spotlight query handle
 * @param[out]    sparql_result  Mapped RDF query, string is allocated in
 *                               talloc context of slq
 * @return        0 on success, -1 on error
 **/
int map_spotlight_to_rdf_query(slq_t *slq, gchar **rdf_result, gchar **fts_result)
{
    EC_INIT;
    YY_BUFFER_STATE s = NULL;
    srp_result = NULL;
    srp_fts = NULL;

    srp_slq = slq;
    s = yy_scan_string(slq->slq_qstring);

    EC_ZERO( yyparse() );

EC_CLEANUP:
    if (s)
        yy_delete_buffer(s);
    if (ret == 0) {
        *rdf_result = srp_result;
        *fts_result = srp_fts;
    } else {
        *rdf_result = NULL;
        *fts_result = NULL;
    }
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

    srp_slq = talloc_zero(NULL, slq_t);
    struct vol *vol = talloc_zero(srp_slq, struct vol);
    vol->v_path = "/Volumes/test";
    srp_slq->slq_vol = vol;

    s = yy_scan_string(argv[1]);

    ret = yyparse();

    yy_delete_buffer(s);

    if (ret == 0)
        printf("RDF:\n%s\nFTS: %s\n",
               srp_result ? srp_result : "(empty)",
               srp_fts ? srp_fts : "(none)");
    return 0;
} 
#endif
