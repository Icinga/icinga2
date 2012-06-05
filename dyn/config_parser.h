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
     T_NULL = 260,
     T_IDENTIFIER = 261,
     T_OPEN_PARENTHESIS = 262,
     T_CLOSE_PARENTHESIS = 263,
     T_OPEN_BRACE = 264,
     T_CLOSE_BRACE = 265,
     T_OPEN_BRACKET = 266,
     T_CLOSE_BRACKET = 267,
     T_EQUAL = 268,
     T_PLUS_EQUAL = 269,
     T_MINUS_EQUAL = 270,
     T_MULTIPLY_EQUAL = 271,
     T_DIVIDE_EQUAL = 272,
     T_COMMA = 273,
     T_ABSTRACT = 274,
     T_LOCAL = 275,
     T_OBJECT = 276,
     T_INCLUDE = 277,
     T_INHERITS = 278
   };
#endif
/* Tokens.  */
#define T_STRING 258
#define T_NUMBER 259
#define T_NULL 260
#define T_IDENTIFIER 261
#define T_OPEN_PARENTHESIS 262
#define T_CLOSE_PARENTHESIS 263
#define T_OPEN_BRACE 264
#define T_CLOSE_BRACE 265
#define T_OPEN_BRACKET 266
#define T_CLOSE_BRACKET 267
#define T_EQUAL 268
#define T_PLUS_EQUAL 269
#define T_MINUS_EQUAL 270
#define T_MULTIPLY_EQUAL 271
#define T_DIVIDE_EQUAL 272
#define T_COMMA 273
#define T_ABSTRACT 274
#define T_LOCAL 275
#define T_OBJECT 276
#define T_INCLUDE 277
#define T_INHERITS 278




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 36 "config_parser.yy"

	char *text;
	int num;
	icinga::Variant *variant;
	icinga::ExpressionOperator op;



/* Line 2068 of yacc.c  */
#line 105 "config_parser.h"
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



