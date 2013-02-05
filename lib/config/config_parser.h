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

/* "%code requires" blocks.  */

/* Line 2068 of yacc.c  */
#line 1 "config_parser.yy"

/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-config.h"

using namespace icinga;

#define YYLTYPE DebugInfo




/* Line 2068 of yacc.c  */
#line 68 "config_parser.h"

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_STRING = 258,
     T_STRING_ANGLE = 259,
     T_NUMBER = 260,
     T_NULL = 261,
     T_IDENTIFIER = 262,
     T_EQUAL = 263,
     T_PLUS_EQUAL = 264,
     T_MINUS_EQUAL = 265,
     T_MULTIPLY_EQUAL = 266,
     T_DIVIDE_EQUAL = 267,
     T_TYPE_DICTIONARY = 268,
     T_TYPE_NUMBER = 269,
     T_TYPE_STRING = 270,
     T_TYPE_SCALAR = 271,
     T_TYPE_ANY = 272,
     T_VALIDATOR = 273,
     T_REQUIRE = 274,
     T_ATTRIBUTE = 275,
     T_TYPE = 276,
     T_ABSTRACT = 277,
     T_LOCAL = 278,
     T_OBJECT = 279,
     T_INCLUDE = 280,
     T_LIBRARY = 281,
     T_INHERITS = 282,
     T_PARTIAL = 283
   };
#endif
/* Tokens.  */
#define T_STRING 258
#define T_STRING_ANGLE 259
#define T_NUMBER 260
#define T_NULL 261
#define T_IDENTIFIER 262
#define T_EQUAL 263
#define T_PLUS_EQUAL 264
#define T_MINUS_EQUAL 265
#define T_MULTIPLY_EQUAL 266
#define T_DIVIDE_EQUAL 267
#define T_TYPE_DICTIONARY 268
#define T_TYPE_NUMBER 269
#define T_TYPE_STRING 270
#define T_TYPE_SCALAR 271
#define T_TYPE_ANY 272
#define T_VALIDATOR 273
#define T_REQUIRE 274
#define T_ATTRIBUTE 275
#define T_TYPE 276
#define T_ABSTRACT 277
#define T_LOCAL 278
#define T_OBJECT 279
#define T_INCLUDE 280
#define T_LIBRARY 281
#define T_INHERITS 282
#define T_PARTIAL 283




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 38 "config_parser.yy"

	char *text;
	double num;
	icinga::Value *variant;
	icinga::ExpressionOperator op;
	icinga::TypeSpecifier type;



/* Line 2068 of yacc.c  */
#line 151 "config_parser.h"
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



