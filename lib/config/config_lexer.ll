%{
/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/exception.hpp"

using namespace icinga;

#include "config/config_parser.hh"
#include <sstream>

#define YYLTYPE icinga::CompilerDebugInfo

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
%option noinput nounput

%x C_COMMENT
%x STRING
%x HEREDOC

%%
\"				{
	yyextra->m_LexBuffer.str("");
	yyextra->m_LexBuffer.clear();

	yyextra->m_LocationBegin = *yylloc;

	BEGIN(STRING);
				}

<STRING>\"			{
	BEGIN(INITIAL);

	yylloc->FirstLine = yyextra->m_LocationBegin.FirstLine;
	yylloc->FirstColumn = yyextra->m_LocationBegin.FirstColumn;

	yylval->text = new String(yyextra->m_LexBuffer.str());

	return T_STRING;
				}

<STRING>\n			{
	BOOST_THROW_EXCEPTION(ScriptError("Unterminated string literal", DebugInfoRange(yyextra->m_LocationBegin, *yylloc)));
				}

<STRING>\\[0-7]{1,3}		{
	/* octal escape sequence */
	int result;

	(void) sscanf(yytext + 1, "%o", &result);

	if (result > 0xff) {
		/* error, constant is out-of-bounds */
		BOOST_THROW_EXCEPTION(ScriptError("Constant is out of bounds: " + String(yytext), *yylloc));
	}

	yyextra->m_LexBuffer << static_cast<char>(result);
				}

<STRING>\\[0-9]+		{
	/* generate error - bad escape sequence; something
	 * like '\48' or '\0777777'
	 */
	BOOST_THROW_EXCEPTION(ScriptError("Bad escape sequence found: " + String(yytext), *yylloc));
				}
<STRING>\\n			{ yyextra->m_LexBuffer << "\n"; }
<STRING>\\\\			{ yyextra->m_LexBuffer << "\\"; }
<STRING>\\\"			{ yyextra->m_LexBuffer << "\""; }
<STRING>\\t			{ yyextra->m_LexBuffer << "\t"; }
<STRING>\\r			{ yyextra->m_LexBuffer << "\r"; }
<STRING>\\b			{ yyextra->m_LexBuffer << "\b"; }
<STRING>\\f			{ yyextra->m_LexBuffer << "\f"; }
<STRING>\\\n			{ yyextra->m_LexBuffer << yytext[1]; }
<STRING>\\.			{
	BOOST_THROW_EXCEPTION(ScriptError("Bad escape sequence found: " + String(yytext), *yylloc));
				}

<STRING>[^\\\n\"]+		{
	char *yptr = yytext;

	while (*yptr)
		yyextra->m_LexBuffer << *yptr++;
				}

<STRING><<EOF>>			{
	BOOST_THROW_EXCEPTION(ScriptError("End-of-file while in string literal", DebugInfoRange(yyextra->m_LocationBegin, *yylloc)));
				}

\{\{\{				{
	yyextra->m_LexBuffer.str("");
	yyextra->m_LexBuffer.clear();

	yyextra->m_LocationBegin = *yylloc;

	BEGIN(HEREDOC);
				}

<HEREDOC>\}\}\}			{
	BEGIN(INITIAL);

	yylloc->FirstLine = yyextra->m_LocationBegin.FirstLine;
	yylloc->FirstColumn = yyextra->m_LocationBegin.FirstColumn;

	yylval->text = new String(yyextra->m_LexBuffer.str());

	return T_STRING;
				}

<HEREDOC>(.|\n)			{ yyextra->m_LexBuffer << yytext[0]; }

<INITIAL>{
"/*"				BEGIN(C_COMMENT);
}

<C_COMMENT>{
"*/"				BEGIN(INITIAL);
[^*]				/* ignore comment */
"*"				/* ignore star */
}

<C_COMMENT><<EOF>>              {
	BOOST_THROW_EXCEPTION(ScriptError("End-of-file while in comment", *yylloc));
				}


\/\/[^\n]*			/* ignore C++-style comments */
#[^\n]*				/* ignore shell-style comments */
[ \t]				/* ignore whitespace */

<INITIAL>{
object				return T_OBJECT;
template			return T_TEMPLATE;
include				return T_INCLUDE;
include_recursive		return T_INCLUDE_RECURSIVE;
include_zones			return T_INCLUDE_ZONES;
library				return T_LIBRARY;
null				return T_NULL;
true				{ yylval->boolean = 1; return T_BOOLEAN; }
false				{ yylval->boolean = 0; return T_BOOLEAN; }
const				return T_CONST;
var				return T_VAR;
this				return T_THIS;
globals				return T_GLOBALS;
locals				return T_LOCALS;
use				return T_USE;
__using				return T_USING;
apply				return T_APPLY;
default				return T_DEFAULT;
to				return T_TO;
where				return T_WHERE;
import				return T_IMPORT;
assign				return T_ASSIGN;
ignore				return T_IGNORE;
function			return T_FUNCTION;
return				return T_RETURN;
break				return T_BREAK;
continue			return T_CONTINUE;
for				return T_FOR;
if				return T_IF;
else				return T_ELSE;
while				return T_WHILE;
throw				return T_THROW;
try				return T_TRY;
except				return T_EXCEPT;
ignore_on_error			return T_IGNORE_ON_ERROR;
current_filename		return T_CURRENT_FILENAME;
current_line			return T_CURRENT_LINE;
debugger			return T_DEBUGGER;
=\>				return T_FOLLOWS;
\<\<				return T_SHIFT_LEFT;
\>\>				return T_SHIFT_RIGHT;
\<=				return T_LESS_THAN_OR_EQUAL;
\>=				return T_GREATER_THAN_OR_EQUAL;
==				return T_EQUAL;
!=				return T_NOT_EQUAL;
!in				return T_NOT_IN;
in				return T_IN;
&&				return T_LOGICAL_AND;
\|\|				return T_LOGICAL_OR;
\{\{				return T_NULLARY_LAMBDA_BEGIN;
\}\}				return T_NULLARY_LAMBDA_END;
[a-zA-Z_][a-zA-Z0-9\_]*		{ yylval->text = new String(yytext); return T_IDENTIFIER; }
@[a-zA-Z_][a-zA-Z0-9\_]*	{ yylval->text = new String(yytext + 1); return T_IDENTIFIER; }
\<[^ \>]*\>			{ yytext[yyleng-1] = '\0'; yylval->text = new String(yytext + 1); return T_STRING_ANGLE; }
[0-9]+(\.[0-9]+)?ms		{ yylval->num = strtod(yytext, NULL) / 1000; return T_NUMBER; }
[0-9]+(\.[0-9]+)?d		{ yylval->num = strtod(yytext, NULL) * 60 * 60 * 24; return T_NUMBER; }
[0-9]+(\.[0-9]+)?h		{ yylval->num = strtod(yytext, NULL) * 60 * 60; return T_NUMBER; }
[0-9]+(\.[0-9]+)?m		{ yylval->num = strtod(yytext, NULL) * 60; return T_NUMBER; }
[0-9]+(\.[0-9]+)?s		{ yylval->num = strtod(yytext, NULL); return T_NUMBER; }
[0-9]+(\.[0-9]+)?		{ yylval->num = strtod(yytext, NULL); return T_NUMBER; }
=				{ yylval->csop = OpSetLiteral; return T_SET; }
\+=				{ yylval->csop = OpSetAdd; return T_SET_ADD; }
-=				{ yylval->csop = OpSetSubtract; return T_SET_SUBTRACT; }
\*=				{ yylval->csop = OpSetMultiply; return T_SET_MULTIPLY; }
\/=				{ yylval->csop = OpSetDivide; return T_SET_DIVIDE; }
\%=				{ yylval->csop = OpSetModulo; return T_SET_MODULO; }
\^=				{ yylval->csop = OpSetXor; return T_SET_XOR; }
\&=				{ yylval->csop = OpSetBinaryAnd; return T_SET_BINARY_AND; }
\|=				{ yylval->csop = OpSetBinaryOr; return T_SET_BINARY_OR; }
\+				return T_PLUS;
\-				return T_MINUS;
\*				return T_MULTIPLY;
\/				return T_DIVIDE_OP;
\%				return T_MODULO;
\^				return T_XOR;
\&				return T_BINARY_AND;
\|				return T_BINARY_OR;
\<				return T_LESS_THAN;
\>				return T_GREATER_THAN;
}

\(				{ yyextra->m_IgnoreNewlines.push(true); return '('; }
\)				{ yyextra->m_IgnoreNewlines.pop(); return ')'; }
[\r\n]+				{ yycolumn -= strlen(yytext) - 1; if (!yyextra->m_IgnoreNewlines.top()) { return T_NEWLINE; } }
<<EOF>>				{ if (!yyextra->m_Eof) { yyextra->m_Eof = true; return T_NEWLINE; } else { yyterminate(); } }
.				return yytext[0];

%%

void ConfigCompiler::InitializeScanner()
{
	yylex_init(&m_Scanner);
	yyset_extra(this, m_Scanner);
}

void ConfigCompiler::DestroyScanner()
{
	yylex_destroy(m_Scanner);
}
