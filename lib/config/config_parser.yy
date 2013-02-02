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
	icinga::TypeSpecifier type;
}

%token <text> T_STRING
%token <text> T_STRING_ANGLE
%token <num> T_NUMBER
%token T_NULL
%token <text> T_IDENTIFIER
%token <op> T_EQUAL
%token <op> T_PLUS_EQUAL
%token <op> T_MINUS_EQUAL
%token <op> T_MULTIPLY_EQUAL
%token <op> T_DIVIDE_EQUAL
%token <type> T_TYPE_DICTIONARY
%token <type> T_TYPE_NUMBER
%token <type> T_TYPE_STRING
%token <type> T_TYPE_SCALAR
%token <type> T_TYPE_ANY
%token T_TYPE
%token T_ABSTRACT
%token T_LOCAL
%token T_OBJECT
%token T_INCLUDE
%token T_LIBRARY
%token T_INHERITS
%token T_PARTIAL
%type <text> identifier
%type <variant> simplevalue
%type <variant> value
%type <variant> expressionlist
%type <variant> typerulelist
%type <op> operator
%type <type> type
%type <num> partial_specifier
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

static stack<TypeRuleList::Ptr> m_RuleLists;
static ConfigType::Ptr m_Type;

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

statement: object | type | include | library
	;

include: T_INCLUDE T_STRING
	{
		context->HandleInclude($2, false);
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		context->HandleInclude($2, true);
	}

library: T_LIBRARY T_STRING
	{
		context->HandleLibrary($2);
	}

identifier: T_IDENTIFIER
	| T_STRING
	{
		$$ = $1;
	}
	;

type: partial_specifier T_TYPE identifier
	{
		String name = String($3);
		m_Type = context->GetTypeByName(name);
		
		if (!m_Type) {
			if ($1)
				throw_exception(invalid_argument("partial type definition for unknown type '" + name + "'"));

			m_Type = boost::make_shared<ConfigType>(name, yylloc);
			context->AddType(m_Type);
		}
	}
	type_inherits_specifier typerulelist
	{
		TypeRuleList::Ptr ruleList = *$6;
		m_Type->GetRuleList()->AddRules(ruleList);
		delete $6;
		
		std::cout << "Created ConfigType: " << m_Type->GetName() << " with " << m_Type->GetRuleList()->GetLength() << " top-level rules." << std::endl;
	}
	;

partial_specifier: /* Empty */
	{
		$$ = 0;
	}
	| T_PARTIAL
	{
		$$ = 1;
	}
	;

typerulelist: '{'
	{
		m_RuleLists.push(boost::make_shared<TypeRuleList>());
	}
	typerules
	'}'
	{
		$$ = new Value(m_RuleLists.top());
		m_RuleLists.pop();
	}
	;

typerules: typerules_inner
	| typerules_inner ','

typerules_inner: /* empty */
	| typerule
	| typerules_inner ',' typerule
	;

typerule: type identifier
	{
		TypeRule rule($1, $2, TypeRuleList::Ptr(), yylloc);
		m_RuleLists.top()->AddRule(rule);
	}
	| type identifier typerulelist
	{
		TypeRule rule($1, $2, *$3, yylloc);
		delete $3;
		m_RuleLists.top()->AddRule(rule);
	}
	;

type_inherits_specifier: /* empty */
	| T_INHERITS T_STRING
	{
		m_Type->SetParent($2);
	}
	;

type: T_TYPE_DICTIONARY
	| T_TYPE_NUMBER
	| T_TYPE_STRING
	| T_TYPE_SCALAR
	| T_TYPE_ANY
	{
		$$ = $1;
	}
	;

object: 
	{
		m_Abstract = false;
		m_Local = false;
	}
attributes T_OBJECT identifier T_STRING
	{
		m_Item = boost::make_shared<ConfigItemBuilder>(yylloc);
		m_Item->SetType($4);
		m_Item->SetName($5);
	}
object_inherits_specifier expressionlist
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

object_inherits_list: object_inherits_item
	| object_inherits_list ',' object_inherits_item
	;

object_inherits_item: T_STRING
	{
		m_Item->AddParent($1);
		free($1);
	}
	;

object_inherits_specifier: /* empty */
	| T_INHERITS object_inherits_list
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

expression: identifier operator value
	{
		Expression expr($1, $2, *$3, yylloc);
		free($1);
		delete $3;

		m_ExpressionLists.top()->AddExpression(expr);
	}
	| identifier '[' T_STRING ']' operator value
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
