/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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

#ifndef YY_YY_SPARQL_PARSER_H_INCLUDED
# define YY_YY_SPARQL_PARSER_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     WORD = 258,
     BOOL = 259,
     FUNC_INRANGE = 260,
     DATE_ISO = 261,
     OBRACE = 262,
     CBRACE = 263,
     EQUAL = 264,
     UNEQUAL = 265,
     GT = 266,
     LT = 267,
     COMMA = 268,
     QUOTE = 269,
     AND = 270,
     OR = 271
   };
#endif
/* Tokens.  */
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 46 "sparql_parser.y"

    int ival;
    const char *sval;
    bool bval;
    time_t tval;


/* Line 2058 of yacc.c  */
#line 97 "sparql_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */
/* "%code provides" blocks.  */
/* Line 2058 of yacc.c  */
#line 40 "sparql_parser.y"

  #define SPRAW_TIME_OFFSET 978307200
  extern int map_spotlight_to_sparql_query(slq_t *slq, gchar **sparql_result);
  extern slq_t *ssp_slq;


/* Line 2058 of yacc.c  */
#line 129 "sparql_parser.h"

#endif /* !YY_YY_SPARQL_PARSER_H_INCLUDED  */
