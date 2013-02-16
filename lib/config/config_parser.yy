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
%token T_VALIDATOR
%token T_REQUIRE
%token T_ATTRIBUTE
%token T_TYPE
%token T_ABSTRACT
%token T_LOCAL
%token T_OBJECT
%token T_TEMPLATE
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
	ConfigCompilerContext::GetContext()->AddError(false, message.str());
}

int yyparse(ConfigCompiler *context);

static stack<ExpressionList::Ptr> m_ExpressionLists;
static Dictionary::Ptr m_Array;
static ConfigItemBuilder::Ptr m_Item;
static bool m_Abstract;
static bool m_Local;

static stack<TypeRuleList::Ptr> m_RuleLists;
static ConfigType::Ptr m_Type;

void ConfigCompiler::Compile(void)
{
	assert(ConfigCompilerContext::GetContext() != NULL);

	try {
		yyparse(this);
	} catch (const exception& ex) {
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
		free($1);
	}
	;

type: partial_specifier T_TYPE identifier
	{
		String name = String($3);
		free($3);

		m_Type = ConfigCompilerContext::GetContext()->GetType(name);

		if (!m_Type) {
			if ($1)
				BOOST_THROW_EXCEPTION(invalid_argument("Partial type definition for unknown type '" + name + "'"));

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
		TypeRule rule($2, $3, TypeRuleList::Ptr(), yylloc);
		free($3);

		m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE type T_STRING typerulelist
	{
		TypeRule rule($2, $3, *$4, yylloc);
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
object_declaration identifier T_STRING
	{
		m_Item = boost::make_shared<ConfigItemBuilder>(yylloc);

		m_Item->SetType($3);
		free($3);

		m_Item->SetName($4);
		free($4);

		m_Item->SetUnit(ConfigCompilerContext::GetContext()->GetUnit());
	}
object_inherits_specifier expressionlist
	{
		ExpressionList::Ptr exprl = *$7;
		delete $7;

		m_Item->AddExpressionList(exprl);
		m_Item->SetLocal(m_Local);
		m_Item->SetAbstract(m_Abstract);

		ConfigCompilerContext::GetContext()->AddItem(m_Item->Compile());
		m_Item.reset();
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
	| value
	{
		Expression expr(String(), OperatorSet, *$1, yylloc);
		delete $1;

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
