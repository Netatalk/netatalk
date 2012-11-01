/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DATE = 258,
     WORD = 259,
     FUNC_INRANGE = 260,
     DATE_SPEC = 261,
     OBRACE = 262,
     CBRACE = 263,
     EQUAL = 264,
     COMMA = 265,
     QUOTE = 266,
     AND = 267,
     OR = 268
   };
#endif
/* Tokens.  */
#define DATE 258
#define WORD 259
#define FUNC_INRANGE 260
#define DATE_SPEC 261
#define OBRACE 262
#define CBRACE 263
#define EQUAL 264
#define COMMA 265
#define QUOTE 266
#define AND 267
#define OR 268




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 33 "spotlight_rawquery_parser.y"

    int ival;
    const char *sval;



/* Line 2068 of yacc.c  */
#line 83 "spotlight_rawquery_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


/* "%code provides" blocks.  */

/* Line 2068 of yacc.c  */
#line 27 "spotlight_rawquery_parser.y"

  extern const gchar *map_spotlight_to_sparql_query(slq_t *slq);
  extern slq_t *ssp_slq;
  extern gchar *ssp_result;



/* Line 2068 of yacc.c  */
#line 105 "spotlight_rawquery_parser.h"
