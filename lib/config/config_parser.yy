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
#include "config/configitembuilder.h"
#include "config/configcompiler.h"
#include "config/configcompilercontext.h"
#include "config/configerror.h"
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
	if (N) {							\
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

static void MakeRBinaryOp(Value** result, AExpression::OpCallback& op, Value *left, Value *right, DebugInfo& diLeft, DebugInfo& diRight)
{
	*result = new Value(make_shared<AExpression>(op, static_cast<AExpression::Ptr>(*left), static_cast<AExpression::Ptr>(*right), DebugInfoRange(diLeft, diRight)));
	delete left;
	delete right;
}

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
	icinga::AExpression::OpCallback op;
	icinga::TypeSpecifier type;
	std::vector<String> *slist;
	Array *array;
}

%token <text> T_STRING
%token <text> T_STRING_ANGLE
%token <num> T_NUMBER
%token T_NULL
%token <text> T_IDENTIFIER

%token <op> T_SET "= (T_SET)"
%token <op> T_SET_PLUS "+= (T_SET_PLUS)"
%token <op> T_SET_MINUS "-= (T_SET_MINUS)"
%token <op> T_SET_MULTIPLY "*= (T_SET_MULTIPLY)"
%token <op> T_SET_DIVIDE "/= (T_SET_DIVIDE)"

%token <op> T_SHIFT_LEFT "<< (T_SHIFT_LEFT)"
%token <op> T_SHIFT_RIGHT ">> (T_SHIFT_RIGHT)"
%token <op> T_EQUAL "== (T_EQUAL)"
%token <op> T_NOT_EQUAL "!= (T_NOT_EQUAL)"
%token <op> T_IN "in (T_IN)"
%token <op> T_NOT_IN "!in (T_NOT_IN)"
%token <op> T_LOGICAL_AND "&& (T_LOGICAL_AND)"
%token <op> T_LOGICAL_OR "|| (T_LOGICAL_OR)"
%token <op> T_LESS_THAN_OR_EQUAL "<= (T_LESS_THAN_OR_EQUAL)"
%token <op> T_GREATER_THAN_OR_EQUAL ">= (T_GREATER_THAN_OR_EQUAL)"
%token <op> T_PLUS "+ (T_PLUS)"
%token <op> T_MINUS "- (T_MINUS)"
%token <op> T_MULTIPLY "* (T_MULTIPLY)"
%token <op> T_DIVIDE_OP "/ (T_DIVIDE_OP)"
%token <op> T_BINARY_AND "& (T_BINARY_AND)"
%token <op> T_BINARY_OR "| (T_BINARY_OR)"
%token <op> T_LESS_THAN "< (T_LESS_THAN)"
%token <op> T_GREATER_THAN "> (T_GREATER_THAN)"

%token T_VAR "var (T_VAR)"
%token T_CONST "const (T_CONST)"
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
%token T_WHERE "where (T_WHERE)"
%token T_IMPORT "import (T_IMPORT)"
%token T_ASSIGN "assign (T_ASSIGN)"
%token T_IGNORE "ignore (T_IGNORE)"

%type <text> identifier
%type <array> rterm_items
%type <array> rterm_items_inner
%type <array> lterm_items
%type <array> lterm_items_inner
%type <variant> typerulelist
%type <op> lbinary_op
%type <type> type
%type <num> partial_specifier
%type <variant> rterm
%type <variant> rterm_scope
%type <variant> lterm
%type <num> variable_decl

%left T_LOGICAL_OR
%left T_LOGICAL_AND
%left T_BINARY_OR
%left T_BINARY_AND
%left T_IN
%left T_NOT_IN
%left T_EQUAL T_NOT_EQUAL
%left T_LESS_THAN T_LESS_THAN_OR_EQUAL T_GREATER_THAN T_GREATER_THAN_OR_EQUAL
%left T_SHIFT_LEFT T_SHIFT_RIGHT
%left T_PLUS T_MINUS
%left T_MULTIPLY T_DIVIDE_OP
%right '!' '~'
%left '.' '(' '['
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigCompiler *, const char *err)
{
	std::ostringstream message;
	message << *locp << ": " << err;
	ConfigCompilerContext::GetInstance()->AddMessage(true, message.str(), *locp);
}

int yyparse(ConfigCompiler *context);

static bool m_Abstract;

static std::stack<TypeRuleList::Ptr> m_RuleLists;
static ConfigType::Ptr m_Type;

static Dictionary::Ptr m_ModuleScope;

static bool m_Apply;
static AExpression::Ptr m_Assign;
static AExpression::Ptr m_Ignore;

void ConfigCompiler::Compile(void)
{
	m_ModuleScope = make_shared<Dictionary>();

	try {
		yyparse(this);
	} catch (const ConfigError& ex) {
		const DebugInfo *di = boost::get_error_info<errinfo_debuginfo>(ex);
		ConfigCompilerContext::GetInstance()->AddMessage(true, ex.what(), di ? *di : DebugInfo());
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
	{ }
	| lterm
	{
		AExpression::Ptr aexpr = *$1;
		aexpr->Evaluate(m_ModuleScope);
		delete $1;
	}
	;

include: T_INCLUDE rterm
	{
		AExpression::Ptr aexpr = static_cast<AExpression::Ptr>(*$2);
		delete $2;

		context->HandleInclude(aexpr->Evaluate(m_ModuleScope), false, DebugInfoRange(@1, @2));
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		context->HandleInclude($2, true, DebugInfoRange(@1, @2));
		free($2);
	}
	;

include_recursive: T_INCLUDE_RECURSIVE rterm
	{
		AExpression::Ptr aexpr = static_cast<AExpression::Ptr>(*$2);
		delete $2;

		context->HandleIncludeRecursive(aexpr->Evaluate(m_ModuleScope), "*.conf", DebugInfoRange(@1, @2));
	}
	| T_INCLUDE_RECURSIVE rterm rterm
	{
		AExpression::Ptr aexpr1 = static_cast<AExpression::Ptr>(*$2);
		delete $2;

		AExpression::Ptr aexpr2 = static_cast<AExpression::Ptr>(*$3);
		delete $3;

		context->HandleIncludeRecursive(aexpr1->Evaluate(m_ModuleScope), aexpr2->Evaluate(m_ModuleScope), DebugInfoRange(@1, @3));
	}
	;

library: T_LIBRARY T_STRING
	{
		context->HandleLibrary($2);
		free($2);
	}
	;

variable: variable_decl identifier T_SET rterm
	{
		AExpression::Ptr aexpr = static_cast<AExpression::Ptr>(*$4);
		delete $4;

		ScriptVariable::Ptr sv = ScriptVariable::Set($2, aexpr->Evaluate(m_ModuleScope));
		sv->SetConstant(true);

		free($2);
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
				BOOST_THROW_EXCEPTION(ConfigError("Partial type definition for unknown type '" + name + "'") << errinfo_debuginfo(DebugInfoRange(@1, @3)));

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
	object_declaration identifier rterm rterm_scope
	{
		DebugInfo di = DebugInfoRange(@2, @5);
		ConfigItemBuilder::Ptr item = make_shared<ConfigItemBuilder>(di);

		AExpression::Ptr aexpr = static_cast<AExpression::Ptr>(*$4);
		delete $4;
		
		String name = aexpr->Evaluate(m_ModuleScope);

		ConfigItem::Ptr oldItem = ConfigItem::GetObject($3, name);

		if (oldItem) {
			std::ostringstream msgbuf;
			msgbuf << "Object '" << name << "' of type '" << $3 << "' re-defined: " << di << "; previous definition: " << oldItem->GetDebugInfo();
			free($3);
			delete $5;
			BOOST_THROW_EXCEPTION(ConfigError(msgbuf.str()) << errinfo_debuginfo(di));
		}

		item->SetType($3);

		if (name.FindFirstOf("!") != String::NPos) {
			std::ostringstream msgbuf;
			msgbuf << "Name for object '" << name << "' of type '" << $3 << "' is invalid: Object names may not contain '!'";
			free($3);
			BOOST_THROW_EXCEPTION(ConfigError(msgbuf.str()) << errinfo_debuginfo(@4));
		}

		free($3);

		item->SetName(name);

		AExpression::Ptr exprl = static_cast<AExpression::Ptr>(*$5);
		delete $5;

		exprl->MakeInline();
		item->AddExpression(exprl);

		item->SetAbstract(m_Abstract);

		item->SetScope(m_ModuleScope);

		item->Compile()->Register();
		item.reset();
	}
	;

object_declaration: T_OBJECT
	| T_TEMPLATE
	{
		m_Abstract = true;
	}

lbinary_op: T_SET
	| T_SET_PLUS
	| T_SET_MINUS
	| T_SET_MULTIPLY
	| T_SET_DIVIDE
	{
		$$ = $1;
	}
	;

comma_or_semicolon: ',' | ';'
	;

lterm_items: lterm_items_inner
	{
		$$ = $1;
	}
	| lterm_items_inner comma_or_semicolon
	{
		$$ = $1;
	}

lterm_items_inner: /* empty */
	{
		$$ = new Array();
	}
	| lterm
	{
		$$ = new Array();
		$$->Add(*$1);
		delete $1;
	}
	| lterm_items_inner comma_or_semicolon lterm
	{
		if ($1)
			$$ = $1;
		else
			$$ = new Array();

		$$->Add(*$3);
		delete $3;
	}
	;

lterm: identifier lbinary_op rterm
	{
		AExpression::Ptr aindex = make_shared<AExpression>(&AExpression::OpLiteral, $1, @1);
		free($1);

		AExpression::Ptr aexpr = static_cast<AExpression::Ptr>(*$3);
		$$ = new Value(make_shared<AExpression>($2, aindex, aexpr, DebugInfoRange(@1, @3)));
		delete $3;
	}
	| identifier '[' rterm ']' lbinary_op rterm
	{
		AExpression::Ptr subexpr = make_shared<AExpression>($5, static_cast<AExpression::Ptr>(*$3), static_cast<AExpression::Ptr>(*$6), DebugInfoRange(@1, @6));
		delete $3;
		delete $6;

		Array::Ptr subexprl = make_shared<Array>();
		subexprl->Add(subexpr);
		
		AExpression::Ptr aindex = make_shared<AExpression>(&AExpression::OpLiteral, $1, @1);
		free($1);

		AExpression::Ptr expr = make_shared<AExpression>(&AExpression::OpDict, subexprl, DebugInfoRange(@1, @6));
		$$ = new Value(make_shared<AExpression>(&AExpression::OpSetPlus, aindex, expr, DebugInfoRange(@1, @6)));
	}
	| identifier '.' T_IDENTIFIER lbinary_op rterm
	{
		AExpression::Ptr aindex = make_shared<AExpression>(&AExpression::OpLiteral, $1, @1);
		AExpression::Ptr subexpr = make_shared<AExpression>($4, aindex, static_cast<AExpression::Ptr>(*$5), DebugInfoRange(@1, @5));
		free($3);
		delete $5;

		Array::Ptr subexprl = make_shared<Array>();
		subexprl->Add(subexpr);

		AExpression::Ptr aindexl = make_shared<AExpression>(&AExpression::OpLiteral, $1, @1);
		free($1);

		AExpression::Ptr expr = make_shared<AExpression>(&AExpression::OpDict, subexprl, DebugInfoRange(@1, @5));
		$$ = new Value(make_shared<AExpression>(&AExpression::OpSetPlus, aindexl, expr, DebugInfoRange(@1, @5)));
	}
	| T_IMPORT rterm
	{
		AExpression::Ptr avar = make_shared<AExpression>(&AExpression::OpVariable, "type", DebugInfoRange(@1, @2));
		AExpression::Ptr aexpr = static_cast<AExpression::Ptr>(*$2);
		delete $2;
		$$ = new Value(make_shared<AExpression>(&AExpression::OpImport, avar, aexpr, DebugInfoRange(@1, @2)));
	}
	| T_ASSIGN T_WHERE rterm
	{
		if (!m_Apply)
			BOOST_THROW_EXCEPTION(ConfigError("'assign' keyword not valid in this context."));

		m_Assign = make_shared<AExpression>(&AExpression::OpLogicalOr, m_Assign, static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3));
		delete $3;

		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, Empty, DebugInfoRange(@1, @3)));
	}
	| T_IGNORE T_WHERE rterm
	{
		if (!m_Apply)
			BOOST_THROW_EXCEPTION(ConfigError("'ignore' keyword not valid in this context."));

		m_Ignore = make_shared<AExpression>(&AExpression::OpLogicalOr, m_Ignore, static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @3));

		delete $3;

		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, Empty, DebugInfoRange(@1, @3)));
	}
	| rterm
	{
		$$ = $1;
	}
	;
	
rterm_items: rterm_items_inner
	{
		$$ = $1;
	}
	| rterm_items_inner ','
	{
		$$ = $1;
	}
	;

rterm_items_inner: /* empty */
	{
		$$ = new Array();
	}
	| rterm
	{
		$$ = new Array();
		$$->Add(*$1);
		delete $1;
	}
	| rterm_items_inner ',' rterm
	{
		if ($1)
			$$ = $1;
		else
			$$ = new Array();

		$$->Add(*$3);
		delete $3;
	}
	;

rterm_scope: '{' lterm_items '}'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpDict, Array::Ptr($2), DebugInfoRange(@1, @3)));
	}
	;

rterm: T_STRING
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, $1, @1));
		free($1);
	}
	| T_NUMBER
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, $1, @1));
	}
	| T_NULL
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, Empty, @1));
	}
	| rterm '.' T_IDENTIFIER
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpIndexer, static_cast<AExpression::Ptr>(*$1), make_shared<AExpression>(&AExpression::OpLiteral, $3, @3), DebugInfoRange(@1, @3)));
		delete $1;
		free($3);
	}
	| T_IDENTIFIER '(' rterm_items ')'
	{
		Array::Ptr arguments = Array::Ptr($3);
		$$ = new Value(make_shared<AExpression>(&AExpression::OpFunctionCall, $1, make_shared<AExpression>(&AExpression::OpLiteral, arguments, @3), DebugInfoRange(@1, @4)));
		free($1);
	}
	| T_IDENTIFIER
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpVariable, $1, @1));
		free($1);
	}
	| '!' rterm
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpLogicalNegate, static_cast<AExpression::Ptr>(*$2), DebugInfoRange(@1, @2)));
		delete $2;
	}
	| '~' rterm
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpNegate, static_cast<AExpression::Ptr>(*$2), DebugInfoRange(@1, @2)));
		delete $2;
	}
	| rterm '[' rterm ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpIndexer, static_cast<AExpression::Ptr>(*$1), static_cast<AExpression::Ptr>(*$3), DebugInfoRange(@1, @4)));
		delete $1;
		free($3);
	}
	| '[' rterm_items ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpArray, Array::Ptr($2), DebugInfoRange(@1, @3)));
	}
	| rterm_scope
	{
		$$ = $1;
	}
	| '(' rterm ')'
	{
		$$ = $2;
	}
	| rterm T_LOGICAL_OR rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_LOGICAL_AND rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_BINARY_OR rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_BINARY_AND rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_IN rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_NOT_IN rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_EQUAL rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_NOT_EQUAL rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_LESS_THAN rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_LESS_THAN_OR_EQUAL rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_GREATER_THAN rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_GREATER_THAN_OR_EQUAL rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_SHIFT_LEFT rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_SHIFT_RIGHT rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_PLUS rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_MINUS rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_MULTIPLY rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	| rterm T_DIVIDE_OP rterm { MakeRBinaryOp(&$$, $2, $1, $3, @1, @3); }
	;

apply:
	{
		m_Apply = true;
		m_Assign = make_shared<AExpression>(&AExpression::OpLiteral, false, DebugInfo());
		m_Ignore = make_shared<AExpression>(&AExpression::OpLiteral, false, DebugInfo());
	}
	T_APPLY identifier rterm rterm
	{
		m_Apply = false;

		AExpression::Ptr aname = static_cast<AExpression::Ptr>(*$4);
		delete $4;
		String type = $3;
		free($3);
		String name = aname->Evaluate(m_ModuleScope);

		if (!ApplyRule::IsValidType(type))
			BOOST_THROW_EXCEPTION(ConfigError("'apply' cannot be used with type '" + type + "'") << errinfo_debuginfo(DebugInfoRange(@2, @3)));

		AExpression::Ptr exprl = static_cast<AExpression::Ptr>(*$5);
		delete $5;

		exprl->MakeInline();

		// assign && !ignore
		AExpression::Ptr rex = make_shared<AExpression>(&AExpression::OpLogicalNegate, m_Ignore, DebugInfoRange(@2, @5));
		AExpression::Ptr filter = make_shared<AExpression>(&AExpression::OpLogicalAnd, m_Assign, rex, DebugInfoRange(@2, @5));
		ApplyRule::AddRule(type, name, exprl, filter, DebugInfoRange(@2, @5), m_ModuleScope);

		m_Assign.reset();
		m_Ignore.reset();
	}
%%
