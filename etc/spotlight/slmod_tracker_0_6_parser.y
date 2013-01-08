%code top {
  #include <atalk/standards.h>

  #include <stdbool.h>
  #include <stdio.h>
  #include <string.h>
  #include <time.h>

  #include <gio/gio.h>
  #include <tracker.h>

  #include <atalk/talloc.h>
  #include <atalk/logger.h>
  #include <atalk/errchk.h>
  #include <atalk/spotlight.h>

  #include "slmod_tracker_0_6_map.h"

  struct yy_buffer_state;
  typedef struct yy_buffer_state *YY_BUFFER_STATE;
  extern int yylex (void);
  extern void yyerror (char const *);
  extern void *yyterminate(void);
  extern YY_BUFFER_STATE yy_scan_string( const char *str);
  extern void yy_delete_buffer ( YY_BUFFER_STATE buffer );

  /* forward declarations */
  static char *map_expr(const char *attr, char op, const char *val);
  static time_t isodate2unix(const char *s);
 
 /* global vars, eg needed by the lexer */
  slq_t *ts_slq;

  /* local vars */
  static ServiceType ts_type;   /* Tracker query object type */
  static gchar *ts_search;      /* Tracker query search term */
}

%code provides {
  #define SPRAW_TIME_OFFSET 978307200
  extern int map_spotlight_to_tracker_0_6_query(slq_t *slq, ServiceType *type, gchar **search);
  extern slq_t *ts_slq;
}

%union {
    int ival;
    char *sval;
    bool bval;
    time_t tval;
}

%expect 5
%error-verbose

%type <sval> match function expr line
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
    ts_search = $1;
    $$ = $1;
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
        YYABORT;
    else
        $$ = $1;
}
| match                        {$$ = $1;}
| function                     {$$ = $1;}
| OBRACE expr CBRACE           {$$ = $2;}
| expr AND expr                {
    if ($1)
        $$ = $1;
    else if ($3)
        $$ = $3;
    else
        YYABORT;
}
| expr OR expr                 {YYABORT;}
;

match:
WORD EQUAL QUOTE WORD QUOTE          {$$ = map_expr($1, '=', $4);}
| WORD UNEQUAL QUOTE WORD QUOTE      {YYABORT;}
| WORD LT QUOTE WORD QUOTE           {YYABORT;}
| WORD GT QUOTE WORD QUOTE           {YYABORT;}
| WORD EQUAL QUOTE WORD QUOTE WORD   {$$ = map_expr($1, '=', $4);}
| WORD UNEQUAL QUOTE WORD QUOTE WORD {YYABORT;}
| WORD LT QUOTE WORD QUOTE WORD      {YYABORT;}
| WORD GT QUOTE WORD QUOTE WORD      {YYABORT;}
;

function:
FUNC_INRANGE OBRACE WORD COMMA date COMMA date CBRACE {YYABORT;}
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

static void map_type_search(const char *val)
{
    for (struct MDTypeMap *p = MDTypeMap; p->mdtm_value; p++) {
        if (strcmp(p->mdtm_value, val) == 0) {
            if (p->mdtm_type == -1)
                ts_type = SERVICE_OTHER_FILES;
            else
                ts_type = p->mdtm_type;
            return;
        }
    }
    ts_type = SERVICE_OTHER_FILES;
}

static char *map_expr(const char *attr, char op, const char *val)
{
    EC_INIT;
    bstring q, search, replace;
    char *result = NULL;

    for (struct spotlight_tracker_map *p = spotlight_tracker_map; p->stm_spotlight_attr; p++) {
        if (strcmp(p->stm_spotlight_attr, attr) == 0) {
            switch (p->stm_type) {
            case stmt_name:
            case stmt_fts:
                q = bfromcstr(val);
                search = bfromcstr("*");
                replace = bfromcstr("");
                bfindreplace(q, search, replace, 0);
                result = talloc_strdup(ts_slq, bdata(q));
                bdestroy(q);
                bdestroy(search);
                bdestroy(replace);
                break;

            case stmt_type:
                map_type_search(val);
                return NULL;

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
 * Map a Spotlight RAW query string to a Tracker 0.6 query
 *
 * @param[in]     slq      Spotlight query handle
 * @param[out]    type     mapped file type
 * @param[out]    search   mapped search term
 * @return        0 on success, -1 on error
 **/
int map_spotlight_to_tracker_0_6_query(slq_t *slq_in,
                                       ServiceType *type,
                                       gchar **search)
{
    EC_INIT;
    YY_BUFFER_STATE s = NULL;

    ts_slq = slq_in;
    s = yy_scan_string(ts_slq->slq_qstring);

    /* Default object type is file */
    *type = SERVICE_FILES;
    *search = NULL;

    EC_ZERO( yyparse() );
    *type = ts_type;
    *search = ts_search;

EC_CLEANUP:
    if (s)
        yy_delete_buffer(s);
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

    ts_slq = talloc_zero(NULL, slq_t);
    struct vol *vol = talloc_zero(ts_slq, struct vol);
    vol->v_path = "/Volumes/test";
    ts_slq->slq_vol = vol;

    s = yy_scan_string(argv[1]);

    ret = yyparse();

    yy_delete_buffer(s);

    if (ret == 0) {
        printf("Tracker 0.6 query: service: %s, searchterm: %s\n",
               tracker_type_to_service_name(ts_type), ts_search);
    }

    return 0;
} 
#endif
