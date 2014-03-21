%{
 #define YYDEBUG 1
 
/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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
#include "config/aexpression.h"
#include "config/applyrule.h"
#include "base/value.h"
#include "base/utility.h"
#include "base/array.h"
#include "base/scriptvariable.h"
#include "base/exception.h"
#include "base/dynamictype.h"
#include <sstream>
#include <stack>
#include <boost/foreach.hpp>

#define YYLTYPE icinga::DebugInfo
#define YYERROR_VERBOSE

#define YYLLOC_DEFAULT(Current, Rhs, N)					\
do {									\
	if (YYID (N)) {							\
		(Current).Path = YYRHSLOC(Rhs, 1).Path;			\
		(Current).FirstLine = YYRHSLOC(Rhs, 1).FirstLine;	\
		(Current).FirstColumn = YYRHSLOC(Rhs, 1).FirstColumn;	\
		(Current).LastLine = YYRHSLOC(Rhs, N).LastLine;		\
		(Current).LastColumn = YYRHSLOC(Rhs, N).LastColumn;	\
	} else {							\
		(Current).Path = YYRHSLOC(Rhs, 0).Path;			\
		(Current).FirstLine = (Current).LastLine =		\
		YYRHSLOC(Rhs, 0).LastLine;				\
		(Current).FirstColumn = (Current).LastColumn =		\
		YYRHSLOC(Rhs, 0).LastColumn;				\
	}								\
} while (0)

#define YY_LOCATION_PRINT(file, loc)			\
do {							\
       std::ostringstream msgbuf;			\
       msgbuf << loc;					\
       std::string str = msgbuf.str();			\
       fputs(str.c_str(), file);			\
} while (0)

using namespace icinga;

%}

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
	Value *aexpr;
}

%token <text> T_STRING
%token <text> T_STRING_ANGLE
%token <num> T_NUMBER
%token T_NULL
%token <text> T_IDENTIFIER
%token <op> T_SET "= (T_SET)"
%token <op> T_PLUS_EQUAL "+= (T_PLUS_EQUAL)"
%token <op> T_MINUS_EQUAL "-= (T_MINUS_EQUAL)"
%token <op> T_MULTIPLY_EQUAL "*= (T_MULTIPLY_EQUAL)"
%token <op> T_DIVIDE_EQUAL "/= (T_DIVIDE_EQUAL)"
%token T_VAR "var (T_VAR)"
%token T_CONST "const (T_CONST)"
%token T_SHIFT_LEFT "<< (T_SHIFT_LEFT)"
%token T_SHIFT_RIGHT ">> (T_SHIFT_RIGHT)"
%token T_EQUAL "== (T_EQUAL)"
%token T_NOT_EQUAL "!= (T_NOT_EQUAL)"
%token T_IN "in (T_IN)"
%token T_NOT_IN "!in (T_NOT_IN)"
%token T_LOGICAL_AND "&& (T_LOGICAL_AND)"
%token T_LOGICAL_OR "|| (T_LOGICAL_OR)"
%token T_LESS_THAN_OR_EQUAL "<= (T_LESS_THAN_OR_EQUAL)"
%token T_GREATER_THAN_OR_EQUAL ">= (T_GREATER_THAN_OR_EQUAL)"
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
%token T_OBJECT "object (T_OBJECT)"
%token T_TEMPLATE "template (T_TEMPLATE)"
%token T_INCLUDE "include (T_INCLUDE)"
%token T_INCLUDE_RECURSIVE "include_recursive (T_INCLUDE_RECURSIVE)"
%token T_LIBRARY "library (T_LIBRARY)"
%token T_INHERITS "inherits (T_INHERITS)"
%token T_PARTIAL "partial (T_PARTIAL)"
%token T_APPLY "apply (T_APPLY)"
%token T_TO "to (T_TO)"
%token T_WHERE "where (T_WHERE)"
%type <text> identifier
%type <array> array_items
%type <array> array_items_inner
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
%type <aexpr> aexpression
%type <num> variable_decl
%left T_LOGICAL_OR
%left T_LOGICAL_AND
%left T_IN
%left T_NOT_IN
%nonassoc T_EQUAL
%nonassoc T_NOT_EQUAL
%left '+' '-'
%left '*' '/'
%left '&'
%left '|'
%right '~'
%right '!'
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigCompiler *, const char *err)
{
	std::ostringstream message;
	message << *locp << ": " << err;
	ConfigCompilerContext::GetInstance()->AddMessage(true, message.str());
}

int yyparse(ConfigCompiler *context);

static std::stack<Array::Ptr> m_Arrays;
static bool m_Abstract;

static std::stack<TypeRuleList::Ptr> m_RuleLists;
static ConfigType::Ptr m_Type;

void ConfigCompiler::Compile(void)
{
	try {
		yyparse(this);
	} catch (const std::exception& ex) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, DiagnosticInformation(ex));
	}
}

#define scanner (context->GetScanner())

%}

%%
statements: /* empty */
	| statements statement
	;

statement: object | type | include | include_recursive | library | variable | apply
	;

include: T_INCLUDE value
	{
		context->HandleInclude(*$2, false, DebugInfoRange(@1, @2));
		delete $2;
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		context->HandleInclude($2, true, DebugInfoRange(@1, @2));
		free($2);
	}
	;

include_recursive: T_INCLUDE_RECURSIVE value
	{
		context->HandleIncludeRecursive(*$2, "*.conf", DebugInfoRange(@1, @2));
		delete $2;
	}
	| T_INCLUDE_RECURSIVE value value
	{
		context->HandleIncludeRecursive(*$2, *$3, DebugInfoRange(@1, @3));
		delete $2;
		delete $3;
	}
	;

library: T_LIBRARY T_STRING
	{
		context->HandleLibrary($2);
		free($2);
	}
	;

variable: variable_decl identifier T_SET value
	{
		Value *value = $4;
		if (value->IsObjectType<ExpressionList>()) {
			Dictionary::Ptr dict = make_shared<Dictionary>();
			ExpressionList::Ptr exprl = *value;
			exprl->Execute(dict);
			delete value;
			value = new Value(dict);
		}

		ScriptVariable::Ptr sv = ScriptVariable::Set($2, *value);
		sv->SetConstant(true);

		free($2);
		delete value;
	}
	;

variable_decl: T_VAR
	{
		$$ = true;
	}
	| T_CONST
	{
		$$ = false;
	}
	;

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

		m_Type = ConfigType::GetByName(name);

		if (!m_Type) {
			if ($1)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Partial type definition for unknown type '" + name + "'"));

			m_Type = make_shared<ConfigType>(name, DebugInfoRange(@1, @3));
			m_Type->Register();
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
		m_RuleLists.push(make_shared<TypeRuleList>());
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
		TypeRule rule($2, String(), $3, TypeRuleList::Ptr(), DebugInfoRange(@1, @3));
		free($3);

		m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE T_TYPE_NAME '(' identifier ')' T_STRING
	{
		TypeRule rule($2, $4, $6, TypeRuleList::Ptr(), DebugInfoRange(@1, @6));
		free($4);
		free($6);

		m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE type T_STRING typerulelist
	{
		TypeRule rule($2, String(), $3, *$4, DebugInfoRange(@1, @4));
		free($3);
		delete $4;
		m_RuleLists.top()->AddRule(rule);
	}
	;

type_inherits_specifier: /* empty */
	| T_INHERITS identifier
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
	}
	object_declaration identifier T_STRING object_inherits_specifier expressionlist
	{
		ConfigItemBuilder::Ptr item = make_shared<ConfigItemBuilder>(DebugInfoRange(@2, @6));

		ConfigItem::Ptr oldItem = ConfigItem::GetObject($3, $4);

		if (oldItem) {
			std::ostringstream msgbuf;
			msgbuf << "Object '" << $4 << "' of type '" << $3 << "' re-defined; previous definition: " << oldItem->GetDebugInfo();
			free($3);
			free($4);
			delete $5;
			BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
		}

		item->SetType($3);

		if (strchr($4, '!') != NULL) {
			std::ostringstream msgbuf;
			msgbuf << "Name for object '" << $4 << "' of type '" << $3 << "' is invalid: Object names may not contain '!'";
			free($3);
			BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
		}

		free($3);

		item->SetName($4);
		free($4);

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

		item->SetAbstract(m_Abstract);

		item->Compile()->Register();
		item.reset();
	}
	;

object_declaration: T_OBJECT
	| T_TEMPLATE
	{
		m_Abstract = true;
	}

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
		if ($2)
			$$ = $2;
		else
			$$ = new ExpressionList();
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
		$$ = new Expression($1, $2, *$3, DebugInfoRange(@1, @3));
		free($1);
		delete $3;
	}
	| identifier '[' T_STRING ']' operator value
	{
		Expression subexpr($3, $5, *$6, DebugInfoRange(@1, @6));
		free($3);
		delete $6;

		ExpressionList::Ptr subexprl = make_shared<ExpressionList>();
		subexprl->AddExpression(subexpr);

		$$ = new Expression($1, OperatorPlus, subexprl, DebugInfoRange(@1, @6));
		free($1);
	}
	;

operator: T_SET
	| T_PLUS_EQUAL
	| T_MINUS_EQUAL
	| T_MULTIPLY_EQUAL
	| T_DIVIDE_EQUAL
	{
		$$ = $1;
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
	| aexpression
	{
		$$ = new Array();
		$$->Add(*$1);
		delete $1;
	}
	| array_items_inner ',' aexpression
	{
		if ($1)
			$$ = $1;
		else
			$$ = new Array();

		$$->Add(*$3);
		delete $3;
	}
	;

aexpression: T_STRING
	{
		$$ = new Value(make_shared<AExpression>(AEReturn, AValue(ATSimple, $1), @1));
		free($1);
	}
	| T_NUMBER
	{
		$$ = new Value(make_shared<AExpression>(AEReturn, AValue(ATSimple, $1), @1));
	}
	| T_NULL
	{
		$$ = new Value(make_shared<AExpression>(AEReturn, AValue(ATSimple, Empty), @1));
	}
	| T_IDENTIFIER '(' array_items ')'
	{
		Array::Ptr arguments = Array::Ptr($3);
		$$ = new Value(make_shared<AExpression>(AEFunctionCall, AValue(ATSimple, $1), AValue(ATSimple, arguments), DebugInfoRange(@1, @4)));
		free($1);
	}
	| T_IDENTIFIER
	{
		$$ = new Value(make_shared<AExpression>(AEReturn, AValue(ATVariable, $1), @1));
		free($1);
	}
	| '!' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AENegate, static_cast<AExpression::Ptr>(*$2), DebugInfoRange(@1, @2)));
		delete $2;
	}
	| '~' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AENegate, static_cast<AExpression::Ptr>(*$2), DebugInfoRange(@1, @2)));
		delete $2;
	}
	| '[' array_items ']'
	{
		$$ = new Value(make_shared<AExpression>(AEArray, AValue(ATSimple, Array::Ptr($2)), DebugInfoRange(@1, @3)));
	}
	| '(' aexpression ')'
	{
		$$ = $2;
	}
	| aexpression '+' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEAdd, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '-' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AESubtract, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '*' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEMultiply, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '/' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEDivide, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '&' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEBinaryAnd, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '|' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEBinaryOr, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @2)));
		delete $1;
		delete $3;
	}
	| aexpression T_IN aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEIn, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_NOT_IN aexpression
	{
		$$ = new Value(make_shared<AExpression>(AENotIn, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_LESS_THAN_OR_EQUAL aexpression
	{
		$$ = new Value(make_shared<AExpression>(AELessThanOrEqual, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_GREATER_THAN_OR_EQUAL aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEGreaterThanOrEqual, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '<' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AELessThan, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression '>' aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEGreaterThan, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_EQUAL aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEEqual, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_NOT_EQUAL aexpression
	{
		$$ = new Value(make_shared<AExpression>(AENotEqual, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_SHIFT_LEFT aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEShiftLeft, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_SHIFT_RIGHT aexpression
	{
		$$ = new Value(make_shared<AExpression>(AEShiftRight, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_LOGICAL_AND aexpression
	{
		$$ = new Value(make_shared<AExpression>(AELogicalAnd, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	| aexpression T_LOGICAL_OR aexpression
	{
		$$ = new Value(make_shared<AExpression>(AELogicalOr, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3)));
		delete $1;
		delete $3;
	}
	;

value: expressionlist
	{
		ExpressionList::Ptr exprl = ExpressionList::Ptr($1);
		$$ = new Value(exprl);
	}
	| aexpression
	{
		AExpression::Ptr aexpr = *$1;
		$$ = new Value(aexpr->Evaluate(Dictionary::Ptr()));
		delete $1;
	}
	;

optional_template: /* empty */
	| T_TEMPLATE
	;

apply: T_APPLY optional_template identifier identifier T_TO identifier T_WHERE aexpression
	{
		if (!ApplyRule::IsValidCombination($3, $6)) {
			BOOST_THROW_EXCEPTION(std::invalid_argument("'apply' cannot be used with types '" + String($3) + "' and '" + String($6) + "'."));
		}

		Array::Ptr arguments = make_shared<Array>();
		arguments->Add(*$8);
		delete $8;

		AExpression::Ptr aexpr = make_shared<AExpression>(AEFunctionCall, AValue(ATSimple, "bool"), AValue(ATSimple, arguments), @8);

		ApplyRule::AddRule($3, $4, $6, aexpr, DebugInfoRange(@1, @8));
	}
%%
