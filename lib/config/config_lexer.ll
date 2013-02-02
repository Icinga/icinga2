%{
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
#include "config_parser.h"

using namespace icinga;

#define YY_EXTRA_TYPE ConfigCompiler *
#define YY_USER_ACTION 					\
do {							\
	yylloc->Path = yyextra->GetPath();		\
	yylloc->FirstLine = yylineno;			\
	yylloc->FirstColumn = yycolumn;			\
	yylloc->LastLine = yylineno;			\
	yylloc->LastColumn = yycolumn + yyleng - 1;	\
	yycolumn += yyleng;				\
} while (0);

#define YY_INPUT(buf, result, max_size)			\
do {							\
	result = yyextra->ReadInput(buf, max_size);	\
} while (0)
%}

%option reentrant noyywrap yylineno
%option bison-bridge bison-locations
%option never-interactive nounistd

%x IN_C_COMMENT

%%
type				return T_TYPE;
dictionary			{ yylval->type = TypeDictionary; return T_TYPE_DICTIONARY; }
number				{ yylval->type = TypeNumber; return T_TYPE_NUMBER; }
string				{ yylval->type = TypeString; return T_TYPE_STRING; }
scalar				{ yylval->type = TypeScalar; return T_TYPE_SCALAR; }
any				{ yylval->type = TypeAny; return T_TYPE_ANY; }
abstract			return T_ABSTRACT;
local				return T_LOCAL;
object				return T_OBJECT;
#include			return T_INCLUDE;
#library			return T_LIBRARY;
inherits			return T_INHERITS;
null				return T_NULL;
partial				return T_PARTIAL;
true				{ yylval->num = 1; return T_NUMBER; }
false				{ yylval->num = 0; return T_NUMBER; }
[a-zA-Z_\*][:a-zA-Z0-9\-_\*]*	{ yylval->text = strdup(yytext); return T_IDENTIFIER; }
\"[^\"]*\"			{ yytext[yyleng-1] = '\0'; yylval->text = strdup(yytext + 1); return T_STRING; }
\<[^\>]*\>			{ yytext[yyleng-1] = '\0'; yylval->text = strdup(yytext + 1); return T_STRING_ANGLE; }
-?[0-9]+(\.[0-9]+)?h		{ yylval->num = strtod(yytext, NULL) * 60 * 60; return T_NUMBER; }
-?[0-9]+(\.[0-9]+)?m		{ yylval->num = strtod(yytext, NULL) * 60; return T_NUMBER; }
-?[0-9]+(\.[0-9]+)?s		{ yylval->num = strtod(yytext, NULL); return T_NUMBER; }
-?[0-9]+(\.[0-9]+)?		{ yylval->num = strtod(yytext, NULL); return T_NUMBER; }
=				{ yylval->op = OperatorSet; return T_EQUAL; }
\+=				{ yylval->op = OperatorPlus; return T_PLUS_EQUAL; }
-=				{ yylval->op = OperatorMinus; return T_MINUS_EQUAL; }
\*=				{ yylval->op = OperatorMultiply; return T_MULTIPLY_EQUAL; }
\/=				{ yylval->op = OperatorDivide; return T_DIVIDE_EQUAL; }

<INITIAL>{
"/*"				BEGIN(IN_C_COMMENT);
}

<IN_C_COMMENT>{
"*/"				BEGIN(INITIAL);
[^*]+				/* ignore comment */
"*"				/* ignore star */
}

\/\/[^\n]+			/* ignore C++-style comments */
[ \t\r\n]+			/* ignore whitespace */

.				return yytext[0];
%%


void ConfigCompiler::InitializeScanner(void)
{
	yylex_init(&m_Scanner);
	yyset_extra(this, m_Scanner);
}

void ConfigCompiler::DestroyScanner(void)
{
	yylex_destroy(m_Scanner);
}

