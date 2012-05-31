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
     T_STRING = 258,
     T_NUMBER = 259,
     T_IDENTIFIER = 260,
     T_OPEN_BRACE = 261,
     T_CLOSE_BRACE = 262,
     T_OPEN_BRACKET = 263,
     T_CLOSE_BRACKET = 264,
     T_EQUAL = 265,
     T_PLUS_EQUAL = 266,
     T_MINUS_EQUAL = 267,
     T_MULTIPLY_EQUAL = 268,
     T_DIVIDE_EQUAL = 269,
     T_COMMA = 270,
     T_ABSTRACT = 271,
     T_LOCAL = 272,
     T_OBJECT = 273,
     T_INCLUDE = 274,
     T_INHERITS = 275
   };
#endif
/* Tokens.  */
#define T_STRING 258
#define T_NUMBER 259
#define T_IDENTIFIER 260
#define T_OPEN_BRACE 261
#define T_CLOSE_BRACE 262
#define T_OPEN_BRACKET 263
#define T_CLOSE_BRACKET 264
#define T_EQUAL 265
#define T_PLUS_EQUAL 266
#define T_MINUS_EQUAL 267
#define T_MULTIPLY_EQUAL 268
#define T_DIVIDE_EQUAL 269
#define T_COMMA 270
#define T_ABSTRACT 271
#define T_LOCAL 272
#define T_OBJECT 273
#define T_INCLUDE 274
#define T_INHERITS 275




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 37 "config_parser.yy"

	char *text;
	int num;
	icinga::Variant *variant;
	icinga::DynamicDictionaryOperator op;



/* Line 2068 of yacc.c  */
#line 99 "config_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



