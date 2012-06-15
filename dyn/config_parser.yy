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

#include "i2-dyn.h"

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
	int num;
	icinga::Variant *variant;
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
%type <variant> tuple
%type <variant> expressionlist
%type <op> operator
%left '+' '-'
%left '*' '/'
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigCompiler *context, const char *err)
{
	stringstream message;

	message << locp->first_line << ":" << locp->first_column
	    << "-"
	    << locp->last_line << ":" << locp->last_column
	    << ": " << err << endl;

	throw runtime_error(message.str());
}

int yyparse(ConfigCompiler *context);

static stack<ExpressionList::Ptr> m_ExpressionLists;
static vector<ConfigItem::Ptr> m_Objects;
static ConfigItem::Ptr m_Object;
static bool m_Abstract;
static bool m_Local;
static Dictionary::Ptr m_Array;

void ConfigCompiler::Compile(void)
{
	m_Objects.clear();
	yyparse(this);
	SetResult(m_Objects);
	m_Objects.clear();
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
	;

object: 
	{
		m_Abstract = false;
		m_Local = false;
	}
attributes T_OBJECT T_IDENTIFIER T_STRING
	{
		m_Object = boost::make_shared<ConfigItem>($4, $5, yylloc);
		free($4);
		free($5);
	}
inherits_specifier expressionlist
	{
		Object::Ptr exprl_object = *$8;
		delete $8;
		ExpressionList::Ptr exprl = dynamic_pointer_cast<ExpressionList>(exprl_object);

		Expression typeexpr("__type", OperatorSet, m_Object->GetType(), yylloc);
		exprl->AddExpression(typeexpr);

		Expression nameexpr("__name", OperatorSet, m_Object->GetName(), yylloc);
		exprl->AddExpression(nameexpr);

		Expression abstractexpr("__abstract", OperatorSet, m_Abstract ? 1 : 0, yylloc);
		exprl->AddExpression(abstractexpr);

		Expression localexpr("__local", OperatorSet, m_Local ? 1 : 0, yylloc);
		exprl->AddExpression(localexpr);

		m_Object->SetExpressionList(exprl);

		m_Objects.push_back(m_Object);
		m_Object.reset();
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
		m_Object->AddParent($1);
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
		$$ = new Variant(m_ExpressionLists.top());
		m_ExpressionLists.pop();
	}
	;

expressions: /* empty */
	| expression
	| expression ',' expressions
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
		$$ = new Variant($1);
		free($1);
	}
	| T_NUMBER
	{
		$$ = new Variant($1);
	}
	| T_NULL
	{
		$$ = new Variant();
	}
	;

value: simplevalue
	| tuple
	| expressionlist
	{
		$$ = $1;
	}
	;

tuple: '('
	{
		m_Array = boost::make_shared<Dictionary>();
	}
	tupleitems
	')'
	{
		$$ = new Variant(m_Array);
		m_Array.reset();
	}
	;

tupleitem: simplevalue
	{
		m_Array->AddUnnamedProperty(*$1);
		delete $1;
	}

tupleitems:
	/* empty */
	| tupleitem
	| tupleitem ',' tupleitems
	;
%%
