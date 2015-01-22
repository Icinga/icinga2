%{
 #define YYDEBUG 1
 
/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "config/typerule.hpp"
#include "config/typerulelist.hpp"
#include "config/expression.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "base/value.hpp"
#include "base/utility.hpp"
#include "base/scriptvariable.hpp"
#include "base/exception.hpp"
#include "base/dynamictype.hpp"
#include "base/configerror.hpp"
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

template<typename T>
static void MakeRBinaryOp(Expression** result, Expression *left, Expression *right, DebugInfo& diLeft, DebugInfo& diRight)
{
	*result = new T(left, right, DebugInfoRange(diLeft, diRight));
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
	icinga::Expression *expr;
	icinga::Value *variant;
	CombinedSetOp csop;
	icinga::TypeSpecifier type;
	std::vector<String> *slist;
	std::vector<Expression *> *elist;
}

%token T_NEWLINE "new-line"
%token <text> T_STRING
%token <text> T_STRING_ANGLE
%token <num> T_NUMBER
%token T_NULL
%token <text> T_IDENTIFIER

%token <csop> T_SET "= (T_SET)"
%token <csop> T_SET_ADD "+= (T_SET_ADD)"
%token <csop> T_SET_SUBTRACT "-= (T_SET_SUBTRACT)"
%token <csop> T_SET_MULTIPLY "*= (T_SET_MULTIPLY)"
%token <csop> T_SET_DIVIDE "/= (T_SET_DIVIDE)"

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
%token T_PLUS "+ (T_PLUS)"
%token T_MINUS "- (T_MINUS)"
%token T_MULTIPLY "* (T_MULTIPLY)"
%token T_DIVIDE_OP "/ (T_DIVIDE_OP)"
%token T_BINARY_AND "& (T_BINARY_AND)"
%token T_BINARY_OR "| (T_BINARY_OR)"
%token T_LESS_THAN "< (T_LESS_THAN)"
%token T_GREATER_THAN "> (T_GREATER_THAN)"

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
%token T_APPLY "apply (T_APPLY)"
%token T_TO "to (T_TO)"
%token T_WHERE "where (T_WHERE)"
%token T_IMPORT "import (T_IMPORT)"
%token T_ASSIGN "assign (T_ASSIGN)"
%token T_IGNORE "ignore (T_IGNORE)"
%token T_APPLY_FOR "for (T_APPLY_FOR)"
%token T_FUNCTION "function (T_FUNCTION)"
%token T_RETURN "return (T_RETURN)"
%token T_FOR "for (T_FOR)"
%token T_FOLLOWS "=> (T_FOLLOWS)"

%type <text> identifier
%type <elist> rterm_items
%type <elist> rterm_items_inner
%type <slist> identifier_items
%type <slist> identifier_items_inner
%type <elist> indexer
%type <elist> indexer_items
%type <expr> indexer_item
%type <elist> lterm_items
%type <elist> lterm_items_inner
%type <variant> typerulelist
%type <csop> combined_set_op
%type <type> type
%type <expr> rterm
%type <expr> rterm_array
%type <expr> rterm_scope
%type <expr> lterm
%type <expr> object
%type <expr> apply
%type <expr> optional_rterm
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

static std::stack<bool> m_Abstract;

static std::stack<TypeRuleList::Ptr> m_RuleLists;
static ConfigType::Ptr m_Type;

static Dictionary::Ptr m_ModuleScope;

static std::stack<bool> m_Apply;
static std::stack<bool> m_ObjectAssign;
static std::stack<bool> m_SeenAssign;
static std::stack<Expression *> m_Assign;
static std::stack<Expression *> m_Ignore;
static std::stack<String> m_FKVar;
static std::stack<String> m_FVVar;
static std::stack<Expression *> m_FTerm;

void ConfigCompiler::Compile(void)
{
	m_ModuleScope = new Dictionary();

	m_Abstract = std::stack<bool>();
	m_RuleLists = std::stack<TypeRuleList::Ptr>();
	m_Type.reset();
	m_Apply = std::stack<bool>();
	m_ObjectAssign = std::stack<bool>();
	m_SeenAssign = std::stack<bool>();
	m_Assign = std::stack<Expression *>();
	m_Ignore = std::stack<Expression *>();
	m_FKVar = std::stack<String>();
	m_FVVar = std::stack<String>();
	m_FTerm = std::stack<Expression *>();

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

statement: type | include | include_recursive | library | constant
	{ }
	| newlines
	{ }
	| lterm
	{
		$1->Evaluate(m_ModuleScope);
		delete $1;
	}
	;

include: T_INCLUDE rterm sep
	{
		context->HandleInclude($2->Evaluate(m_ModuleScope), false, DebugInfoRange(@1, @2));
		delete $2;
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		context->HandleInclude($2, true, DebugInfoRange(@1, @2));
		free($2);
	}
	;

include_recursive: T_INCLUDE_RECURSIVE rterm
	{
		context->HandleIncludeRecursive($2->Evaluate(m_ModuleScope), "*.conf", DebugInfoRange(@1, @2));
		delete $2;
	}
	| T_INCLUDE_RECURSIVE rterm ',' rterm
	{
		context->HandleIncludeRecursive($2->Evaluate(m_ModuleScope), $4->Evaluate(m_ModuleScope), DebugInfoRange(@1, @4));
		delete $2;
		delete $4;
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
		ScriptVariable::Ptr sv = ScriptVariable::Set($2, $4->Evaluate(m_ModuleScope));
		free($2);
		delete $4;

		sv->SetConstant(true);
	}
	;

identifier: T_IDENTIFIER
	| T_STRING
	{
		$$ = $1;
	}
	;

type: T_TYPE identifier
	{
		String name = String($2);
		free($2);

		m_Type = ConfigType::GetByName(name);

		if (!m_Type) {
			m_Type = new ConfigType(name, DebugInfoRange(@1, @2));
			m_Type->Register();
		}
	}
	type_inherits_specifier typerulelist sep
	{
		TypeRuleList::Ptr ruleList = *$5;
		delete $5;

		m_Type->GetRuleList()->AddRules(ruleList);
		m_Type->GetRuleList()->AddRequires(ruleList);

		String validator = ruleList->GetValidator();
		if (!validator.IsEmpty())
			m_Type->GetRuleList()->SetValidator(validator);
	}
	;

typerulelist: '{'
	{
		m_RuleLists.push(new TypeRuleList());
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
		m_Abstract.push(false);
		m_ObjectAssign.push(true);
		m_SeenAssign.push(false);
		m_Assign.push(NULL);
		m_Ignore.push(NULL);
	}
	object_declaration identifier rterm rterm_scope
	{
		m_ObjectAssign.pop();

		bool abstract = m_Abstract.top();
		m_Abstract.pop();

		String type = $3;
		free($3);

		DictExpression *exprl = dynamic_cast<DictExpression *>($5);
		exprl->MakeInline();

		bool seen_assign = m_SeenAssign.top();
		m_SeenAssign.pop();

		Expression *ignore = m_Ignore.top();
		m_Ignore.pop();

		Expression *assign = m_Assign.top();
		m_Assign.pop();

		Expression *filter = NULL;

		if (seen_assign) {
			if (!ObjectRule::IsValidSourceType(type))
				BOOST_THROW_EXCEPTION(ConfigError("object rule 'assign' cannot be used for type '" + type + "'") << errinfo_debuginfo(DebugInfoRange(@2, @3)));

			if (ignore) {
				Expression *rex = new LogicalNegateExpression(ignore, DebugInfoRange(@2, @5));

				filter = new LogicalAndExpression(assign, rex, DebugInfoRange(@2, @5));
			} else
				filter = assign;
		}

		$$ = new ObjectExpression(abstract, type, $4, filter, context->GetZone(), exprl, DebugInfoRange(@2, @5));
	}
	;

object_declaration: T_OBJECT
	| T_TEMPLATE
	{
		m_Abstract.top() = true;
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
		$$ = new std::vector<String>();
	}
	| identifier
	{
		$$ = new std::vector<String>();
		$$->push_back($1);
		free($1);
	}
	| identifier_items_inner ',' identifier
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<String>();

		$$->push_back($3);
		free($3);
	}
	;

indexer: identifier
	{
		$$ = new std::vector<Expression *>();
		$$->push_back(MakeLiteral($1));
		free($1);
	}
	| identifier indexer_items
	{
		$$ = $2;
		$$->insert($$->begin(), MakeLiteral($1));
		free($1);
	}
	;

indexer_items: indexer_item
	{
		$$ = new std::vector<Expression *>();
		$$->push_back($1);
	}
	| indexer_items indexer_item
	{
		$$ = $1;
		$$->push_back($2);
	}
	;

indexer_item: '.' identifier
	{
		$$ = MakeLiteral($2);
		free($2);
	}
	| '[' rterm ']'
	{
		$$ = $2;
	}
	;

combined_set_op: T_SET
	| T_SET_ADD
	| T_SET_SUBTRACT
	| T_SET_MULTIPLY
	| T_SET_DIVIDE
	{
		$$ = $1;
	}
	;

lterm_items: /* empty */
	{
		$$ = new std::vector<Expression *>();
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
		$$ = new std::vector<Expression *>();
		$$->push_back($1);
	}
	| lterm_items_inner sep lterm
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<Expression *>();

		$$->push_back($3);
	}
	;

lterm: indexer combined_set_op rterm
	{
		$$ = new SetExpression(*$1, $2, $3, DebugInfoRange(@1, @3));
		delete $1;
	}
	| T_IMPORT rterm
	{
		Expression *avar = new VariableExpression("type", DebugInfoRange(@1, @2));
		$$ = new ImportExpression(avar, $2, DebugInfoRange(@1, @2));
	}
	| T_ASSIGN T_WHERE rterm
	{
		if ((m_Apply.empty() || !m_Apply.top()) && (m_ObjectAssign.empty() || !m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ConfigError("'assign' keyword not valid in this context."));

		m_SeenAssign.top() = true;

		if (m_Assign.top())
			m_Assign.top() = new LogicalOrExpression(m_Assign.top(), $3, DebugInfoRange(@1, @3));
		else
			m_Assign.top() = $3;

		$$ = MakeLiteral();
	}
	| T_IGNORE T_WHERE rterm
	{
		if ((m_Apply.empty() || !m_Apply.top()) && (m_ObjectAssign.empty() || !m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ConfigError("'ignore' keyword not valid in this context."));

		if (m_Ignore.top())
			m_Ignore.top() = new LogicalOrExpression(m_Ignore.top(), $3, DebugInfoRange(@1, @3));
		else
			m_Ignore.top() = $3;

		$$ = MakeLiteral();
	}
	| T_RETURN rterm
	{
		std::vector<Expression *> vname;
		vname.push_back(MakeLiteral("__result"));
		$$ = new SetExpression(vname, OpSetLiteral, $2, DebugInfoRange(@1, @2));
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
		$$ = new std::vector<Expression *>();
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
		$$ = new std::vector<Expression *>();
		$$->push_back($1);
	}
	| rterm_items_inner arraysep rterm
	{
		$$ = $1;
		$$->push_back($3);
	}
	;

rterm_array: '[' newlines rterm_items newlines ']'
	{
		$$ = new ArrayExpression(*$3, DebugInfoRange(@1, @5));
		delete $3;
	}
	| '[' newlines rterm_items ']'
	{
		$$ = new ArrayExpression(*$3, DebugInfoRange(@1, @4));
		delete $3;
	}
	| '[' rterm_items newlines ']'
	{
		$$ = new ArrayExpression(*$2, DebugInfoRange(@1, @4));
		delete $2;
	}
	| '[' rterm_items ']'
	{
		$$ = new ArrayExpression(*$2, DebugInfoRange(@1, @3));
		delete $2;
	}
	;

rterm_scope: '{' newlines lterm_items newlines '}'
	{
		$$ = new DictExpression(*$3, DebugInfoRange(@1, @5));
		delete $3;
	}
	| '{' newlines lterm_items '}'
	{
		$$ = new DictExpression(*$3, DebugInfoRange(@1, @4));
		delete $3;
	}
	| '{' lterm_items newlines '}'
	{
		$$ = new DictExpression(*$2, DebugInfoRange(@1, @4));
		delete $2;
	}
	| '{' lterm_items '}'
	{
		$$ = new DictExpression(*$2, DebugInfoRange(@1, @3));
		delete $2;
	}
	;

rterm: T_STRING
	{
		$$ = MakeLiteral($1);
		free($1);
	}
	| T_NUMBER
	{
		$$ = MakeLiteral($1);
	}
	| T_NULL
	{
		$$ = MakeLiteral();
	}
	| rterm '.' T_IDENTIFIER
	{
		$$ = new IndexerExpression($1, MakeLiteral($3), DebugInfoRange(@1, @3));
		free($3);
	}
	| rterm '(' rterm_items ')'
	{
		$$ = new FunctionCallExpression($1, *$3, DebugInfoRange(@1, @4));
		delete $3;
	}
	| T_IDENTIFIER
	{
		$$ = new VariableExpression($1, @1);
		free($1);
	}
	| '!' rterm
	{
		$$ = new LogicalNegateExpression($2, DebugInfoRange(@1, @2));
	}
	| '~' rterm
	{
		$$ = new NegateExpression($2, DebugInfoRange(@1, @2));
	}
	| rterm '[' rterm ']'
	{
		$$ = new IndexerExpression($1, $3, DebugInfoRange(@1, @4));
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
	| rterm T_LOGICAL_OR rterm { MakeRBinaryOp<LogicalOrExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_LOGICAL_AND rterm { MakeRBinaryOp<LogicalAndExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_BINARY_OR rterm { MakeRBinaryOp<BinaryOrExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_BINARY_AND rterm { MakeRBinaryOp<BinaryAndExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_IN rterm { MakeRBinaryOp<InExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_NOT_IN rterm { MakeRBinaryOp<NotInExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_EQUAL rterm { MakeRBinaryOp<EqualExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_NOT_EQUAL rterm { MakeRBinaryOp<NotEqualExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_LESS_THAN rterm { MakeRBinaryOp<LessThanExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_LESS_THAN_OR_EQUAL rterm { MakeRBinaryOp<LessThanOrEqualExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_GREATER_THAN rterm { MakeRBinaryOp<GreaterThanExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_GREATER_THAN_OR_EQUAL rterm { MakeRBinaryOp<GreaterThanOrEqualExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_SHIFT_LEFT rterm { MakeRBinaryOp<ShiftLeftExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_SHIFT_RIGHT rterm { MakeRBinaryOp<ShiftRightExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_PLUS rterm { MakeRBinaryOp<AddExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_MINUS rterm { MakeRBinaryOp<SubtractExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_MULTIPLY rterm { MakeRBinaryOp<MultiplyExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_DIVIDE_OP rterm { MakeRBinaryOp<DivideExpression>(&$$, $1, $3, @1, @3); }
	| T_FUNCTION identifier '(' identifier_items ')' rterm_scope
	{
		DictExpression *aexpr = dynamic_cast<DictExpression *>($6);
		aexpr->MakeInline();

		$$ = new FunctionExpression($2, *$4, aexpr, DebugInfoRange(@1, @6));
		free($2);
		delete $4;
	}
	| T_FUNCTION '(' identifier_items ')' rterm_scope
	{
		DictExpression *aexpr = dynamic_cast<DictExpression *>($5);
		aexpr->MakeInline();

		$$ = new FunctionExpression("", *$3, aexpr, DebugInfoRange(@1, @5));
		delete $3;
	}
	| T_FOR '(' identifier T_FOLLOWS identifier T_IN rterm ')' rterm_scope
	{
		$$ = new ForExpression($3, $5, $7, $9, DebugInfoRange(@1, @9));
		free($3);
		free($5);
	}
	| T_FOR '(' identifier T_IN rterm ')' rterm_scope
	{
		$$ = new ForExpression($3, "", $5, $7, DebugInfoRange(@1, @7));
		free($3);
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

apply_for_specifier: /* empty */
	| T_APPLY_FOR '(' identifier T_FOLLOWS identifier T_IN rterm ')'
	{
		m_FKVar.top() = $3;
		free($3);

		m_FVVar.top() = $5;
		free($5);

		m_FTerm.top() = $7;
	}
	| T_APPLY_FOR '(' identifier T_IN rterm ')'
	{
		m_FKVar.top() = $3;
		free($3);

		m_FVVar.top() = "";

		m_FTerm.top() = $5;
	}
	;

optional_rterm: /* empty */
	{
		$$ = MakeLiteral();
	}
	| rterm
	{
		$$ = $1;
	}
	;

apply:
	{
		m_Apply.push(true);
		m_SeenAssign.push(false);
		m_Assign.push(NULL);
		m_Ignore.push(NULL);
		m_FKVar.push("");
		m_FVVar.push("");
		m_FTerm.push(NULL);
	}
	T_APPLY identifier optional_rterm apply_for_specifier target_type_specifier rterm
	{
		m_Apply.pop();

		String type = $3;
		free($3);
		String target = $6;
		free($6);

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

		DictExpression *exprl = dynamic_cast<DictExpression *>($7);
		exprl->MakeInline();

		// assign && !ignore
		if (!m_SeenAssign.top())
			BOOST_THROW_EXCEPTION(ConfigError("'apply' is missing 'assign'") << errinfo_debuginfo(DebugInfoRange(@2, @3)));

		m_SeenAssign.pop();

		Expression *ignore = m_Ignore.top();
		m_Ignore.pop();

		Expression *assign = m_Assign.top();
		m_Assign.pop();

		Expression *filter;

		if (ignore) {
			Expression *rex = new LogicalNegateExpression(ignore, DebugInfoRange(@2, @5));

			filter = new LogicalAndExpression(assign, rex, DebugInfoRange(@2, @5));
		} else
			filter = assign;

		String fkvar = m_FKVar.top();
		m_FKVar.pop();

		String fvvar = m_FVVar.top();
		m_FVVar.pop();

		Expression *fterm = m_FTerm.top();
		m_FTerm.pop();

		$$ = new ApplyExpression(type, target, $4, filter, fkvar, fvvar, fterm, exprl, DebugInfoRange(@2, @5));
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
