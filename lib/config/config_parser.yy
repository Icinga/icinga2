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
#include "config/expression.h"
#include "config/expressionlist.h"
#include "config/configitembuilder.h"
#include "config/configcompiler.h"
#include "config/configcompilercontext.h"
#include "config/typerule.h"
#include "config/typerulelist.h"
#include "base/value.h"
#include "base/utility.h"
#include "base/array.h"
#include <sstream>
#include <stack>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

#define YYLTYPE icinga::DebugInfo

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
	std::vector<String> *slist;
	Expression *expr;
	ExpressionList *exprl;
	Array *array;
}

%token <text> T_STRING
%token <text> T_STRING_ANGLE
%token <num> T_NUMBER
%token T_NULL
%token <text> T_IDENTIFIER
%token <op> T_EQUAL "= (T_EQUAL)"
%token <op> T_PLUS_EQUAL "+= (T_PLUS_EQUAL)"
%token <op> T_MINUS_EQUAL "-= (T_MINUS_EQUAL)"
%token <op> T_MULTIPLY_EQUAL "*= (T_MULTIPLY_EQUAL)"
%token <op> T_DIVIDE_EQUAL "/= (T_DIVIDE_EQUAL)"
%token <type> T_TYPE_DICTIONARY "dictionary (T_TYPE_DICTIONARY)"
%token <type> T_TYPE_ARRAY "array (T_TYPE_ARRAY)"
%token <type> T_TYPE_NUMBER "number (T_TYPE_NUMBER)"
%token <type> T_TYPE_STRING "string (T_TYPE_STRING)"
%token <type> T_TYPE_SCALAR "scalar (T_TYPE_SCALAR)"
%token <type> T_TYPE_ANY "any (T_TYPE_ANY)"
%token <type> T_TYPE_NAME "name (T_TYPE_NAME)"
%token T_VALIDATOR "%validator (T_VALIDATOR)"
%token T_REQUIRE "%require (T_REQUIRE)"
%token T_ATTRIBUTE "%attribute (T_ATTRIBUTE)"
%token T_TYPE "type (T_TYPE)"
%token T_ABSTRACT "abstract (T_ABSTRACT)"
%token T_LOCAL "local (T_LOCAL)"
%token T_OBJECT "object (T_OBJECT)"
%token T_TEMPLATE "template (T_TEMPLATE)"
%token T_INCLUDE "include (T_INCLUDE)"
%token T_LIBRARY "library (T_LIBRARY)"
%token T_INHERITS "inherits (T_INHERITS)"
%token T_PARTIAL "partial (T_PARTIAL)"
%type <text> identifier
%type <array> array
%type <array> array_items
%type <array> array_items_inner
%type <variant> simplevalue
%type <variant> value
%type <expr> expression
%type <exprl> expressions
%type <exprl> expressions_inner
%type <exprl> expressionlist
%type <variant> typerulelist
%type <op> operator
%type <type> type
%type <num> partial_specifier
%type <slist> object_inherits_list
%type <slist> object_inherits_specifier
%left '+' '-'
%left '*' '/'
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigCompiler *, const char *err)
{
	std::ostringstream message;
	message << *locp << ": " << err;
	ConfigCompilerContext::GetContext()->AddError(false, message.str());
}

int yyparse(ConfigCompiler *context);

static std::stack<Array::Ptr> m_Arrays;
static bool m_Abstract;
static bool m_Local;

static std::stack<TypeRuleList::Ptr> m_RuleLists;
static ConfigType::Ptr m_Type;

void ConfigCompiler::Compile(void)
{
	ASSERT(ConfigCompilerContext::GetContext() != NULL);

	try {
		yyparse(this);
	} catch (const std::exception& ex) {
		ConfigCompilerContext::GetContext()->AddError(false, boost::diagnostic_information(ex));
	}
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
		context->HandleInclude($2, false, yylloc);
		free($2);
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		context->HandleInclude($2, true, yylloc);
		free($2);
	}

library: T_LIBRARY T_STRING
	{
		context->HandleLibrary($2);
		free($2);
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
		free($3);

		m_Type = ConfigCompilerContext::GetContext()->GetType(name);

		if (!m_Type) {
			if ($1)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Partial type definition for unknown type '" + name + "'"));

			m_Type = boost::make_shared<ConfigType>(name, yylloc);
			ConfigCompilerContext::GetContext()->AddType(m_Type);
		}
	}
	type_inherits_specifier typerulelist
	{
		TypeRuleList::Ptr ruleList = *$6;
		m_Type->GetRuleList()->AddRules(ruleList);
		m_Type->GetRuleList()->AddRequires(ruleList);

		String validator = ruleList->GetValidator();
		if (!validator.IsEmpty())
			m_Type->GetRuleList()->SetValidator(validator);

		delete $6;
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

typerule: T_REQUIRE T_STRING
	{
		m_RuleLists.top()->AddRequire($2);
		free($2);
	}
	| T_VALIDATOR T_STRING
	{
		m_RuleLists.top()->SetValidator($2);
		free($2);
	}
	| T_ATTRIBUTE type T_STRING
	{
		TypeRule rule($2, String(), $3, TypeRuleList::Ptr(), yylloc);
		free($3);

		m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE T_TYPE_NAME '(' identifier ')' T_STRING
	{
		TypeRule rule($2, $4, $6, TypeRuleList::Ptr(), yylloc);
		free($4);
		free($6);

		m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE type T_STRING typerulelist
	{
		TypeRule rule($2, String(), $3, *$4, yylloc);
		free($3);
		delete $4;
		m_RuleLists.top()->AddRule(rule);
	}
	;

type_inherits_specifier: /* empty */
	| T_INHERITS T_STRING
	{
		m_Type->SetParent($2);
		free($2);
	}
	;

type: T_TYPE_DICTIONARY
	| T_TYPE_ARRAY
	| T_TYPE_NUMBER
	| T_TYPE_STRING
	| T_TYPE_SCALAR
	| T_TYPE_ANY
	| T_TYPE_NAME
	{
		$$ = $1;
	}
	;

object:
	{
		m_Abstract = false;
		m_Local = false;
	}
object_declaration identifier T_STRING object_inherits_specifier expressionlist
	{
		ConfigItemBuilder::Ptr item = boost::make_shared<ConfigItemBuilder>(yylloc);

		item->SetType($3);
		free($3);

		if (strchr($4, ':') != NULL)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Object names may not contain ':'"));

		item->SetName($4);
		free($4);

		item->SetUnit(ConfigCompilerContext::GetContext()->GetUnit());

		if ($5) {
			BOOST_FOREACH(const String& parent, *$5) {
				item->AddParent(parent);
			}

			delete $5;
		}

		if ($6) {
			ExpressionList::Ptr exprl = ExpressionList::Ptr($6);
			item->AddExpressionList(exprl);
		}

		item->SetLocal(m_Local);
		item->SetAbstract(m_Abstract);

		ConfigCompilerContext::GetContext()->AddItem(item->Compile());
		item.reset();
	}
	;

object_declaration: attributes T_OBJECT
	| T_TEMPLATE
	{
		m_Abstract = true;
	}

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

object_inherits_list:
	{
		$$ = NULL;
	}
	| T_STRING
	{
		$$ = new std::vector<String>();
		$$->push_back($1);
		free($1);
	}
	| object_inherits_list ',' T_STRING
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<String>();

		$$->push_back($3);
		free($3);
	}
	;

object_inherits_specifier:
	{
		$$ = NULL;
	}
	| T_INHERITS object_inherits_list
	{
		$$ = $2;
	}
	;

expressionlist: '{' expressions	'}'
	{
		$$ = $2;
	}
	;

expressions: expressions_inner
	{
		$$ = $1;
	}
	| expressions_inner ','
	{
		$$ = $1;
	}

expressions_inner: /* empty */
	{
		$$ = NULL;
	}
	| expression
	{
		$$ = new ExpressionList();
		$$->AddExpression(*$1);
		delete $1;
	}
	| expressions_inner ',' expression
	{
		if ($1)
			$$ = $1;
		else
			$$ = new ExpressionList();

		$$->AddExpression(*$3);
		delete $3;
	}
	;

expression: identifier operator value
	{
		$$ = new Expression($1, $2, *$3, yylloc);
		free($1);
		delete $3;
	}
	| identifier '[' T_STRING ']' operator value
	{
		Expression subexpr($3, $5, *$6, yylloc);
		free($3);
		delete $6;

		ExpressionList::Ptr subexprl = boost::make_shared<ExpressionList>();
		subexprl->AddExpression(subexpr);

		$$ = new Expression($1, OperatorPlus, subexprl, yylloc);
		free($1);
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

array: '[' array_items ']'
	{
		$$ = $2;
	}
	;

array_items: array_items_inner
	{
		$$ = $1;
	}
	| array_items_inner ','
	{
		$$ = $1;
	}

array_items_inner: /* empty */
	{
		$$ = NULL;
	}
	| value
	{
		$$ = new Array();
		$$->Add(*$1);
		delete $1;
	}
	| array_items_inner ',' value
	{
		if ($1)
			$$ = $1;
		else
			$$ = new Array();

		$$->Add(*$3);
		delete $3;
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
	| array
	{
		if ($1 == NULL)
			$1 = new Array();

		Array::Ptr array = Array::Ptr($1);
		$$ = new Value(array);
	}
	;

value: simplevalue
	| expressionlist
	{
		ExpressionList::Ptr exprl = ExpressionList::Ptr($1);
		$$ = new Value(exprl);
	}
	;
%%
