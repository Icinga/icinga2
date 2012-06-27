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

#include "i2-dyn.h"
#include "config_parser.h"

using namespace icinga;

#define YY_EXTRA_TYPE ConfigCompiler *
#define YY_USER_ACTION 					\
do {							\
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
abstract			return T_ABSTRACT;
local				return T_LOCAL;
temporary			return T_TEMPORARY;
object				return T_OBJECT;
include				return T_INCLUDE;
inherits			return T_INHERITS;
null				return T_NULL;
[a-zA-Z_][a-zA-Z0-9\-_]*	{ yylval->text = strdup(yytext); return T_IDENTIFIER; }
\"[^\"]+\"			{ yytext[yyleng-1] = '\0'; yylval->text = strdup(yytext + 1); return T_STRING; }
[0-9]+				{ yylval->num = atoi(yytext); return T_NUMBER; }
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
#[^\n]+				/* ignore shell-style comments */
[ \t\n]+			/* ignore whitespace */

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

