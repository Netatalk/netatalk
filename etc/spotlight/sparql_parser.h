/* A Bison parser, made by GNU Bison 3.7.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SPARQL_PARSER_H_INCLUDED
# define YY_YY_SPARQL_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    WORD = 258,                    /* WORD  */
    BOOL = 259,                    /* BOOL  */
    FUNC_INRANGE = 260,            /* FUNC_INRANGE  */
    DATE_ISO = 261,                /* DATE_ISO  */
    OBRACE = 262,                  /* OBRACE  */
    CBRACE = 263,                  /* CBRACE  */
    EQUAL = 264,                   /* EQUAL  */
    UNEQUAL = 265,                 /* UNEQUAL  */
    GT = 266,                      /* GT  */
    LT = 267,                      /* LT  */
    COMMA = 268,                   /* COMMA  */
    QUOTE = 269,                   /* QUOTE  */
    AND = 270,                     /* AND  */
    OR = 271                       /* OR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define WORD 258
#define BOOL 259
#define FUNC_INRANGE 260
#define DATE_ISO 261
#define OBRACE 262
#define CBRACE 263
#define EQUAL 264
#define UNEQUAL 265
#define GT 266
#define LT 267
#define COMMA 268
#define QUOTE 269
#define AND 270
#define OR 271

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 50 "sparql_parser.y"

    int ival;
    const char *sval;
    bool bval;
    time_t tval;

#line 106 "sparql_parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);
/* "%code provides" blocks.  */
#line 44 "sparql_parser.y"

  #define SPRAW_TIME_OFFSET 978307200
  extern int map_spotlight_to_sparql_query(slq_t *slq, gchar **sparql_result);
  extern slq_t *ssp_slq;

#line 125 "sparql_parser.h"

#endif /* !YY_YY_SPARQL_PARSER_H_INCLUDED  */
