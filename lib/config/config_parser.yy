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

#include "config/i2-config.hpp"
#include "config/configitembuilder.hpp"
#include "config/configtype.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configerror.hpp"
#include "config/typerule.hpp"
#include "config/typerulelist.hpp"
#include "config/aexpression.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "config/aexpression.hpp"
#include "base/value.hpp"
#include "base/utility.hpp"
#include "base/array.hpp"
#include "base/scriptvariable.hpp"
#include "base/exception.hpp"
#include "base/dynamictype.hpp"
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

int ignore_newlines = 0;

static void MakeRBinaryOp(Value** result, AExpression::OpCallback& op, Value *left, Value *right, DebugInfo& diLeft, DebugInfo& diRight)
{
	*result = new Value(make_shared<AExpression>(op, *left, *right, DebugInfoRange(diLeft, diRight)));
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

%token T_NEWLINE "new-line"
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
%token T_TO "to (T_TO)"
%token T_WHERE "where (T_WHERE)"
%token T_IMPORT "import (T_IMPORT)"
%token T_ASSIGN "assign (T_ASSIGN)"
%token T_IGNORE "ignore (T_IGNORE)"
%token T_FUNCTION "function (T_FUNCTION)"
%token T_RETURN "return (T_RETURN)"
%token T_ZONE "zone (T_ZONE)"
%token T_FOR "for (T_FOR)"

%type <text> identifier
%type <array> rterm_items
%type <array> rterm_items_inner
%type <array> identifier_items
%type <array> identifier_items_inner
%type <array> lterm_items
%type <array> lterm_items_inner
%type <variant> typerulelist
%type <op> lbinary_op
%type <type> type
%type <num> partial_specifier
%type <variant> rterm
%type <variant> rterm_array
%type <variant> rterm_scope
%type <variant> lterm
%type <variant> object
%type <variant> apply
%type <text> target_type_specifier

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
%right ':'
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
static int m_StatementNum;

static bool m_Apply;
static bool m_ObjectAssign;
static bool m_SeenAssign;
static AExpression::Ptr m_Assign;
static AExpression::Ptr m_Ignore;

void ConfigCompiler::Compile(void)
{
	m_ModuleScope = make_shared<Dictionary>();
	
	int parentStatementNum = m_StatementNum;
	m_StatementNum = 0;

	try {
		yyparse(this);
	} catch (const ConfigError& ex) {
		const DebugInfo *di = boost::get_error_info<errinfo_debuginfo>(ex);
		ConfigCompilerContext::GetInstance()->AddMessage(true, ex.what(), di ? *di : DebugInfo());
	} catch (const std::exception& ex) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, DiagnosticInformation(ex));
	}

	m_StatementNum = parentStatementNum;
}

#define scanner (context->GetScanner())

%}

%%
statements: /* empty */
	| statements statement
	;

statement: type | zone | include | include_recursive | library | constant
	{
		m_StatementNum++;
	}
	| newlines
	{ }
	| lterm
	{
		AExpression::Ptr aexpr = *$1;
		aexpr->Evaluate(m_ModuleScope);
		delete $1;

		m_StatementNum++;
	}
	;

zone: T_ZONE rterm sep
	{
		AExpression::Ptr aexpr = *$2;
		delete $2;

		if (!context->GetZone().IsEmpty())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Zone name cannot be changed once it's been set."));

		if (m_StatementNum != 0)
			BOOST_THROW_EXCEPTION(std::invalid_argument("'zone' directive must be the first statement in a file."));

		context->SetZone(aexpr->Evaluate(m_ModuleScope));
	}
	| T_ZONE rterm
	{
		AExpression::Ptr aexpr = *$2;
		delete $2;

		if (!context->GetZone().IsEmpty())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Zone name cannot be changed once it's been set."));

		context->SetZone(aexpr->Evaluate(m_ModuleScope));
	}
	rterm_scope sep
	{
		AExpression::Ptr ascope = *$4;
		delete $4;

		try {
			ascope->Evaluate(m_ModuleScope);
			context->SetZone(String());
		} catch (...) {
			context->SetZone(String());
		}
	}
	;

include: T_INCLUDE rterm sep
	{
		AExpression::Ptr aexpr = *$2;
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
		AExpression::Ptr aexpr = *$2;
		delete $2;

		context->HandleIncludeRecursive(aexpr->Evaluate(m_ModuleScope), "*.conf", DebugInfoRange(@1, @2));
	}
	| T_INCLUDE_RECURSIVE rterm ',' rterm
	{
		AExpression::Ptr aexpr1 = *$2;
		delete $2;

		AExpression::Ptr aexpr2 = *$4;
		delete $4;

		context->HandleIncludeRecursive(aexpr1->Evaluate(m_ModuleScope), aexpr2->Evaluate(m_ModuleScope), DebugInfoRange(@1, @4));
	}
	;

library: T_LIBRARY T_STRING sep
	{
		context->HandleLibrary($2);
		free($2);
	}
	;

constant: T_CONST identifier T_SET rterm sep
	{
		AExpression::Ptr aexpr = *$4;
		delete $4;

		ScriptVariable::Ptr sv = ScriptVariable::Set($2, aexpr->Evaluate(m_ModuleScope));
		sv->SetConstant(true);

		free($2);
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
	type_inherits_specifier typerulelist sep
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
	| typerules_inner sep

typerules_inner: /* empty */
	| typerule
	| typerules_inner sep typerule
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
		m_ObjectAssign = true;
		m_SeenAssign = false;
		m_Assign = make_shared<AExpression>(&AExpression::OpLiteral, false, DebugInfo());
		m_Ignore = make_shared<AExpression>(&AExpression::OpLiteral, false, DebugInfo());
	}
	object_declaration identifier rterm rterm_scope
	{
		m_ObjectAssign = false;

		Array::Ptr args = make_shared<Array>();
		
		args->Add(m_Abstract);

		String type = $3;
		args->Add(type);
		free($3);

		args->Add(*$4);
		delete $4;

		AExpression::Ptr exprl = *$5;
		delete $5;
		exprl->MakeInline();

		if (m_SeenAssign && !ObjectRule::IsValidSourceType(type))
			BOOST_THROW_EXCEPTION(ConfigError("object rule 'assign' cannot be used for type '" + type + "'") << errinfo_debuginfo(DebugInfoRange(@2, @3)));

		AExpression::Ptr rex = make_shared<AExpression>(&AExpression::OpLogicalNegate, m_Ignore, DebugInfoRange(@2, @5));
		AExpression::Ptr filter = make_shared<AExpression>(&AExpression::OpLogicalAnd, m_Assign, rex, DebugInfoRange(@2, @5));

		args->Add(filter);

		args->Add(context->GetZone());

		$$ = new Value(make_shared<AExpression>(&AExpression::OpObject, args, exprl, DebugInfoRange(@2, @5)));

		m_Assign.reset();
		m_Ignore.reset();
	}
	;

object_declaration: T_OBJECT
	| T_TEMPLATE
	{
		m_Abstract = true;
	}

identifier_items: identifier_items_inner
	{
		$$ = $1;
	}
	| identifier_items_inner ','
	{
		$$ = $1;
	}
	;

identifier_items_inner: /* empty */
	{
		$$ = new Array();
	}
	| identifier
	{
		$$ = new Array();
		$$->Add($1);
		free($1);
	}
	| identifier_items_inner ',' identifier
	{
		if ($1)
			$$ = $1;
		else
			$$ = new Array();

		$$->Add($3);
		free($3);
	}
	;

lbinary_op: T_SET
	| T_SET_PLUS
	| T_SET_MINUS
	| T_SET_MULTIPLY
	| T_SET_DIVIDE
	{
		$$ = $1;
	}
	;

lterm_items: /* empty */
	{
		$$ = new Array();
	}
	| lterm_items_inner
	{
		$$ = $1;
	}
	| lterm_items_inner sep
	{
		$$ = $1;
	}

lterm_items_inner: lterm
	{
		$$ = new Array();
		$$->Add(*$1);
		delete $1;
	}
	| lterm_items_inner sep lterm
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

		$$ = new Value(make_shared<AExpression>($2, aindex, *$3, DebugInfoRange(@1, @3)));
		delete $3;
	}
	| identifier '[' rterm ']' lbinary_op rterm
	{
		AExpression::Ptr subexpr = make_shared<AExpression>($5, *$3, *$6, DebugInfoRange(@1, @6));
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
		AExpression::Ptr aindex = make_shared<AExpression>(&AExpression::OpLiteral, $3, @3);
		AExpression::Ptr subexpr = make_shared<AExpression>($4, aindex, *$5, DebugInfoRange(@1, @5));
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
		$$ = new Value(make_shared<AExpression>(&AExpression::OpImport, avar, *$2, DebugInfoRange(@1, @2)));
		delete $2;
	}
	| T_ASSIGN T_WHERE rterm
	{
		if (!(m_Apply || m_ObjectAssign))
			BOOST_THROW_EXCEPTION(ConfigError("'assign' keyword not valid in this context."));

		m_SeenAssign = true;

		m_Assign = make_shared<AExpression>(&AExpression::OpLogicalOr, m_Assign, *$3, DebugInfoRange(@1, @3));
		delete $3;

		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, Empty, DebugInfoRange(@1, @3)));
	}
	| T_IGNORE T_WHERE rterm
	{
		if (!(m_Apply || m_ObjectAssign))
			BOOST_THROW_EXCEPTION(ConfigError("'ignore' keyword not valid in this context."));

		m_Ignore = make_shared<AExpression>(&AExpression::OpLogicalOr, m_Ignore, *$3, DebugInfoRange(@1, @3));

		delete $3;

		$$ = new Value(make_shared<AExpression>(&AExpression::OpLiteral, Empty, DebugInfoRange(@1, @3)));
	}
	| T_RETURN rterm
	{
		AExpression::Ptr aname = make_shared<AExpression>(&AExpression::OpLiteral, "__result", @1);
		$$ = new Value(make_shared<AExpression>(&AExpression::OpSet, aname, *$2, DebugInfoRange(@1, @2)));
		delete $2;

	}
	| apply
	{
		$$ = $1;
	}
	| object
	{
		$$ = $1;
	}
	| rterm
	{
		$$ = $1;
	}
	;
	
rterm_items: /* empty */
	{
		$$ = new Array();
	}
	| rterm_items_inner
	{
		$$ = $1;
	}
	| rterm_items_inner arraysep
	{
		$$ = $1;
	}
	;

rterm_items_inner: rterm
	{
		$$ = new Array();
		$$->Add(*$1);
		delete $1;
	}
	| rterm_items_inner arraysep rterm
	{
		$$ = $1;
		$$->Add(*$3);
		delete $3;
	}
	;

rterm_array: '[' newlines rterm_items newlines ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpArray, Array::Ptr($3), DebugInfoRange(@1, @5)));
	}
	| '[' newlines rterm_items ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpArray, Array::Ptr($3), DebugInfoRange(@1, @4)));
	}
	| '[' rterm_items newlines ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpArray, Array::Ptr($2), DebugInfoRange(@1, @4)));
	}
	| '[' rterm_items ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpArray, Array::Ptr($2), DebugInfoRange(@1, @3)));
	}
	;

rterm_scope: '{' newlines lterm_items newlines '}'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpDict, Array::Ptr($3), DebugInfoRange(@1, @5)));
	}
	| '{' newlines lterm_items '}'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpDict, Array::Ptr($3), DebugInfoRange(@1, @4)));
	}
	| '{' lterm_items newlines '}'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpDict, Array::Ptr($2), DebugInfoRange(@1, @4)));
	}
	| '{' lterm_items '}'
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
		$$ = new Value(make_shared<AExpression>(&AExpression::OpIndexer, *$1, make_shared<AExpression>(&AExpression::OpLiteral, $3, @3), DebugInfoRange(@1, @3)));
		delete $1;
		free($3);
	}
	| rterm '(' rterm_items ')'
	{
		Array::Ptr arguments = Array::Ptr($3);
		$$ = new Value(make_shared<AExpression>(&AExpression::OpFunctionCall, *$1, make_shared<AExpression>(&AExpression::OpLiteral, arguments, @3), DebugInfoRange(@1, @4)));
		delete $1;
	}
	| T_IDENTIFIER
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpVariable, $1, @1));
		free($1);
	}
	| '!' rterm
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpLogicalNegate, *$2, DebugInfoRange(@1, @2)));
		delete $2;
	}
	| '~' rterm
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpNegate, *$2, DebugInfoRange(@1, @2)));
		delete $2;
	}
	| rterm '[' rterm ']'
	{
		$$ = new Value(make_shared<AExpression>(&AExpression::OpIndexer, *$1, *$3, DebugInfoRange(@1, @4)));
		delete $1;
		delete $3;
	}
	| rterm_array
	{
		$$ = $1;
	}
	| rterm_scope
	{
		$$ = $1;
	}
	| '('
	{
		ignore_newlines++;
	}
	rterm ')'
	{
		ignore_newlines--;
		$$ = $3;
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
	| T_FUNCTION identifier '(' identifier_items ')' rterm_scope
	{
		Array::Ptr arr = make_shared<Array>();

		arr->Add($2);
		free($2);

		AExpression::Ptr aexpr = *$6;
		delete $6;
		aexpr->MakeInline();
		arr->Add(aexpr);

		$$ = new Value(make_shared<AExpression>(&AExpression::OpFunction, arr, Array::Ptr($4), DebugInfoRange(@1, @6)));
	}
	| T_FUNCTION '(' identifier_items ')' rterm_scope
	{
		Array::Ptr arr = make_shared<Array>();

		arr->Add(Empty);

		AExpression::Ptr aexpr = *$5;
		delete $5;
		aexpr->MakeInline();
		arr->Add(aexpr);

		$$ = new Value(make_shared<AExpression>(&AExpression::OpFunction, arr, Array::Ptr($3), DebugInfoRange(@1, @5)));
	}
	| T_FOR '(' identifier T_IN rterm ')' rterm_scope
	{
		Array::Ptr arr = make_shared<Array>();

		arr->Add($3);
		free($3);

		AExpression::Ptr aexpr = *$5;
		delete $5;
		arr->Add(aexpr);

		AExpression::Ptr ascope = *$7;
		delete $7;

		$$ = new Value(make_shared<AExpression>(&AExpression::OpFor, arr, ascope, DebugInfoRange(@1, @7)));
	}
	;

target_type_specifier: /* empty */
	{
		$$ = strdup("");
	}
	| T_TO identifier
	{
		$$ = $2;
	}
	;

apply:
	{
		m_Apply = true;
		m_SeenAssign = false;
		m_Assign = make_shared<AExpression>(&AExpression::OpLiteral, false, DebugInfo());
		m_Ignore = make_shared<AExpression>(&AExpression::OpLiteral, false, DebugInfo());
	}
	T_APPLY identifier rterm target_type_specifier rterm
	{
		m_Apply = false;

		String type = $3;
		free($3);
		AExpression::Ptr aname = *$4;
		delete $4;
		String target = $5;
		free($5);

		if (!ApplyRule::IsValidSourceType(type))
			BOOST_THROW_EXCEPTION(ConfigError("'apply' cannot be used with type '" + type + "'") << errinfo_debuginfo(DebugInfoRange(@2, @3)));

		if (!ApplyRule::IsValidTargetType(type, target)) {
			if (target == "") {
				std::vector<String> types = ApplyRule::GetTargetTypes(type);
				String typeNames;

				for (std::vector<String>::size_type i = 0; i < types.size(); i++) {
					if (typeNames != "") {
						if (i == types.size() - 1)
							typeNames += " or ";
						else
							typeNames += ", ";
					}

					typeNames += "'" + types[i] + "'";
				}

				BOOST_THROW_EXCEPTION(ConfigError("'apply' target type is ambiguous (can be one of " + typeNames + "): use 'to' to specify a type") << errinfo_debuginfo(DebugInfoRange(@2, @3)));
			} else
				BOOST_THROW_EXCEPTION(ConfigError("'apply' target type '" + target + "' is invalid") << errinfo_debuginfo(DebugInfoRange(@2, @5)));
		}

		AExpression::Ptr exprl = *$6;
		delete $6;

		exprl->MakeInline();

		// assign && !ignore
		if (!m_SeenAssign)
			BOOST_THROW_EXCEPTION(ConfigError("'apply' is missing 'assign'") << errinfo_debuginfo(DebugInfoRange(@2, @3)));

		AExpression::Ptr rex = make_shared<AExpression>(&AExpression::OpLogicalNegate, m_Ignore, DebugInfoRange(@2, @5));
		AExpression::Ptr filter = make_shared<AExpression>(&AExpression::OpLogicalAnd, m_Assign, rex, DebugInfoRange(@2, @5));

		Array::Ptr args = make_shared<Array>();
		args->Add(type);
		args->Add(target);
		args->Add(aname);
		args->Add(filter);

		$$ = new Value(make_shared<AExpression>(&AExpression::OpApply, args, exprl, DebugInfoRange(@2, @5)));

		m_Assign.reset();
		m_Ignore.reset();
	}
	;

newlines: T_NEWLINE
	| newlines T_NEWLINE
	;

/* required separator */
sep: ',' newlines
	| ','
	| ';' newlines
	| ';'
	| newlines
	;

arraysep: ',' newlines
	| ','
	;

%%
