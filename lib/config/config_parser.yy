%code requires {
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

}

%pure-parser

%locations
%defines
%error-verbose

%parse-param { ConfigCompiler *context }
%lex-param { void *scanner }

%union {
	char *text;
	double num;
	icinga::Value *variant;
	icinga::ExpressionOperator op;
}

%token <text> T_STRING
%token <num> T_NUMBER
%token T_NULL
%token <text> T_IDENTIFIER
%token <op> T_EQUAL
%token <op> T_PLUS_EQUAL
%token <op> T_MINUS_EQUAL
%token <op> T_MULTIPLY_EQUAL
%token <op> T_DIVIDE_EQUAL
%token T_ABSTRACT
%token T_LOCAL
%token T_OBJECT
%token T_INCLUDE
%token T_INHERITS
%type <variant> simplevalue
%type <variant> value
%type <variant> expressionlist
%type <op> operator
%left '+' '-'
%left '*' '/'
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigCompiler *, const char *err)
{
	stringstream message;
	message << *locp << ": " << err;
	throw_exception(runtime_error(message.str()));
}

int yyparse(ConfigCompiler *context);

static stack<ExpressionList::Ptr> m_ExpressionLists;
static ConfigItemBuilder::Ptr m_Item;
static bool m_Abstract;
static bool m_Local;
static Dictionary::Ptr m_Array;

void ConfigCompiler::Compile(void)
{
	yyparse(this);
}

#define scanner (context->GetScanner())

%}

%%
statements: /* empty */
	| statements statement
	;

statement: object | include
	;

include: T_INCLUDE T_STRING
	{
		context->HandleInclude($2);
	}

object: 
	{
		m_Abstract = false;
		m_Local = false;
	}
attributes T_OBJECT T_IDENTIFIER T_STRING
	{
		m_Item = boost::make_shared<ConfigItemBuilder>(yylloc);
		m_Item->SetType($4);
		m_Item->SetName($5);
	}
inherits_specifier expressionlist
	{
		ExpressionList::Ptr exprl = *$8;
		delete $8;

		m_Item->AddExpressionList(exprl);
		m_Item->SetLocal(m_Local);
		m_Item->SetAbstract(m_Abstract);

		context->AddObject(m_Item->Compile());
		m_Item.reset();
	}
	;

attributes: /* empty */
	| attributes attribute
	;

attribute: T_ABSTRACT
	{
		m_Abstract = true;
	}
	| T_LOCAL
	{
		m_Local = true;
	}
	;

inherits_list: inherits_item
	| inherits_list ',' inherits_item
	;

inherits_item: T_STRING
	{
		m_Item->AddParent($1);
		free($1);
	}
	;

inherits_specifier: /* empty */
	| T_INHERITS inherits_list
	;

expressionlist: '{'
	{
		m_ExpressionLists.push(boost::make_shared<ExpressionList>());
	}
	expressions
	'}'
	{
		$$ = new Value(m_ExpressionLists.top());
		m_ExpressionLists.pop();
	}
	;

expressions: expressions_inner
	| expressions_inner ','

expressions_inner: /* empty */
	| expression
	| expressions_inner ',' expression
	;

expression: T_IDENTIFIER operator value
	{
		Expression expr($1, $2, *$3, yylloc);
		free($1);
		delete $3;

		m_ExpressionLists.top()->AddExpression(expr);
	}
	| T_IDENTIFIER '[' T_STRING ']' operator value
	{
		Expression subexpr($3, $5, *$6, yylloc);
		free($3);
		delete $6;

		ExpressionList::Ptr subexprl = boost::make_shared<ExpressionList>();
		subexprl->AddExpression(subexpr);

		Expression expr($1, OperatorPlus, subexprl, yylloc);
		free($1);

		m_ExpressionLists.top()->AddExpression(expr);
	}
	| T_STRING
	{
		Expression expr($1, OperatorSet, $1, yylloc);
		free($1);

		m_ExpressionLists.top()->AddExpression(expr);
	}
	;

operator: T_EQUAL
	| T_PLUS_EQUAL
	| T_MINUS_EQUAL
	| T_MULTIPLY_EQUAL
	| T_DIVIDE_EQUAL
	{
		$$ = $1;
	}
	;

simplevalue: T_STRING
	{
		$$ = new Value($1);
		free($1);
	}
	| T_NUMBER
	{
		$$ = new Value($1);
	}
	| T_NULL
	{
		$$ = new Value();
	}
	;

value: simplevalue
	| expressionlist
	{
		$$ = $1;
	}
	;
%%
