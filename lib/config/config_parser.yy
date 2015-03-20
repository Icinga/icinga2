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
#include "config/configcompiler.hpp"
#include "config/typerule.hpp"
#include "config/typerulelist.hpp"
#include "config/expression.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "base/value.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/dynamictype.hpp"
#include "base/exception.hpp"
#include <sstream>
#include <stack>
#include <boost/foreach.hpp>

#define YYLTYPE icinga::CompilerDebugInfo
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

template<typename T>
static void MakeRBinaryOp(Expression** result, Expression *left, Expression *right, const DebugInfo& diLeft, const DebugInfo& diRight)
{
	*result = new T(left, right, DebugInfoRange(diLeft, diRight));
}

%}

%pure-parser

%locations
%defines
%error-verbose
%glr-parser

%parse-param { std::vector<std::pair<Expression *, EItemInfo> > *llist }
%parse-param { ConfigCompiler *context }
%lex-param { void *scanner }

%union {
	char *text;
	double num;
	bool boolean;
	icinga::Expression *expr;
	icinga::DictExpression *dexpr;
	icinga::Value *variant;
	CombinedSetOp csop;
	icinga::TypeSpecifier type;
	std::vector<String> *slist;
	std::vector<std::pair<Expression *, EItemInfo> > *llist;
	std::vector<Expression *> *elist;
	std::vector<std::pair<Expression *, Expression *> > *ebranchlist;
	std::pair<Expression *, Expression *> *ebranch;
	std::pair<String, Expression *> *cvitem;
	std::map<String, Expression *> *cvlist;
	icinga::ScopeSpecifier scope;
}

%token T_NEWLINE "new-line"
%token <text> T_STRING
%token <text> T_STRING_ANGLE
%token <num> T_NUMBER
%token <boolean> T_BOOLEAN
%token T_NULL
%token <text> T_IDENTIFIER

%token <csop> T_SET "= (T_SET)"
%token <csop> T_SET_ADD "+= (T_SET_ADD)"
%token <csop> T_SET_SUBTRACT "-= (T_SET_SUBTRACT)"
%token <csop> T_SET_MULTIPLY "*= (T_SET_MULTIPLY)"
%token <csop> T_SET_DIVIDE "/= (T_SET_DIVIDE)"
%token <csop> T_SET_MODULO "%= (T_SET_MODULO)"
%token <csop> T_SET_XOR "^= (T_SET_XOR)"
%token <csop> T_SET_BINARY_AND "&= (T_SET_BINARY_AND)"
%token <csop> T_SET_BINARY_OR "|= (T_SET_BINARY_OR)"

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
%token T_MODULO "% (T_MODULO)"
%token T_XOR "^ (T_XOR)"
%token T_BINARY_AND "& (T_BINARY_AND)"
%token T_BINARY_OR "| (T_BINARY_OR)"
%token T_LESS_THAN "< (T_LESS_THAN)"
%token T_GREATER_THAN "> (T_GREATER_THAN)"

%token T_VAR "var (T_VAR)"
%token T_GLOBALS "globals (T_GLOBALS)"
%token T_LOCALS "locals (T_LOCALS)"
%token T_CONST "const (T_CONST)"
%token T_USE "use (T_USE)"
%token <type> T_TYPE_DICTIONARY "dictionary (T_TYPE_DICTIONARY)"
%token <type> T_TYPE_ARRAY "array (T_TYPE_ARRAY)"
%token <type> T_TYPE_NUMBER "number (T_TYPE_NUMBER)"
%token <type> T_TYPE_STRING "string (T_TYPE_STRING)"
%token <type> T_TYPE_SCALAR "scalar (T_TYPE_SCALAR)"
%token <type> T_TYPE_ANY "any (T_TYPE_ANY)"
%token <type> T_TYPE_FUNCTION "function (T_TYPE_FUNCTION)"
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
%token T_FUNCTION "function (T_FUNCTION)"
%token T_RETURN "return (T_RETURN)"
%token T_BREAK "break (T_BREAK)"
%token T_CONTINUE "continue (T_CONTINUE)"
%token T_FOR "for (T_FOR)"
%token T_IF "if (T_IF)"
%token T_ELSE "else (T_ELSE)"
%token T_WHILE "while (T_WHILE)"
%token T_FOLLOWS "=> (T_FOLLOWS)"
%token T_NULLARY_LAMBDA_BEGIN "{{ (T_NULLARY_LAMBDA_BEGIN)"
%token T_NULLARY_LAMBDA_END "}} (T_NULLARY_LAMBDA_END)"

%type <text> identifier
%type <elist> rterm_items
%type <elist> rterm_items_inner
%type <slist> identifier_items
%type <slist> identifier_items_inner
%type <variant> typerulelist
%type <csop> combined_set_op
%type <type> type
%type <llist> statements
%type <llist> lterm_items
%type <llist> lterm_items_inner
%type <expr> rterm
%type <expr> rterm_array
%type <dexpr> rterm_scope_require_side_effect
%type <dexpr> rterm_scope
%type <ebranchlist> else_if_branches
%type <ebranch> else_if_branch
%type <expr> rterm_side_effect
%type <expr> rterm_no_side_effect
%type <expr> lterm
%type <expr> object
%type <expr> apply
%type <expr> optional_rterm
%type <text> target_type_specifier
%type <cvlist> use_specifier
%type <cvlist> use_specifier_items
%type <cvitem> use_specifier_item
%type <num> object_declaration

%right T_FOLLOWS
%right T_INCLUDE T_INCLUDE_RECURSIVE T_OBJECT T_TEMPLATE T_APPLY T_IMPORT T_ASSIGN T_IGNORE T_WHERE
%right T_FUNCTION T_FOR
%left T_SET T_SET_ADD T_SET_SUBTRACT T_SET_MULTIPLY T_SET_DIVIDE T_SET_MODULO T_SET_XOR T_SET_BINARY_AND T_SET_BINARY_OR
%left T_LOGICAL_OR
%left T_LOGICAL_AND
%left T_RETURN T_BREAK T_CONTINUE
%left T_IDENTIFIER
%left T_BINARY_OR
%left T_XOR
%left T_BINARY_AND
%nonassoc T_EQUAL T_NOT_EQUAL
%left T_IN T_NOT_IN
%nonassoc T_LESS_THAN T_LESS_THAN_OR_EQUAL T_GREATER_THAN T_GREATER_THAN_OR_EQUAL
%left T_SHIFT_LEFT T_SHIFT_RIGHT
%left T_PLUS T_MINUS
%left T_MULTIPLY T_DIVIDE_OP T_MODULO
%left UNARY_MINUS UNARY_PLUS
%right '!' '~'
%left '.' '(' '['
%left T_VAR T_THIS T_GLOBALS T_LOCALS
%right ';' ','
%right T_NEWLINE
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

extern int yydebug;

void yyerror(const YYLTYPE *locp, std::vector<std::pair<Expression *, EItemInfo> > *, ConfigCompiler *context, const char *err)
{
	bool incomplete = context && context->m_Eof && (context->m_OpenBraces > 0);
	BOOST_THROW_EXCEPTION(ScriptError(err, *locp, incomplete));
}

int yyparse(std::vector<std::pair<Expression *, EItemInfo> > *llist, ConfigCompiler *context);

Expression *ConfigCompiler::Compile(void)
{
	std::vector<std::pair<Expression *, EItemInfo> > llist;

	//yydebug = 1;

	if (yyparse(&llist, this) != 0)
		return NULL;

	std::vector<Expression *> dlist;
	typedef std::pair<Expression *, EItemInfo> EListItem;
	int num = 0;
	BOOST_FOREACH(const EListItem& litem, llist) {
		if (!litem.second.SideEffect && num != llist.size() - 1) {
			yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
		}
		dlist.push_back(litem.first);
		num++;
	}

	DictExpression *expr = new DictExpression(dlist);
	expr->MakeInline();
	return expr;
}

#define scanner (context->GetScanner())

%}

%%
script: statements
	{
		llist->swap(*$1);
		delete $1;
	}
	;

statements: newlines lterm_items
	{
		$$ = $2;
	}
	| lterm_items
	;

lterm_items: /* empty */
	{
		$$ = new std::vector<std::pair<Expression *, EItemInfo> >();
	}
	| lterm_items_inner
	| lterm_items_inner sep
	;

lterm_items_inner: lterm %dprec 2
	{
		$$ = new std::vector<std::pair<Expression *, EItemInfo> >();
		EItemInfo info = { true, @1 };
		$$->push_back(std::make_pair($1, info));
	}
	| rterm_no_side_effect
	{
		$$ = new std::vector<std::pair<Expression *, EItemInfo> >();
		EItemInfo info = { false, @1 };
		$$->push_back(std::make_pair($1, info));
	}
	| lterm_items_inner sep lterm %dprec 1
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<std::pair<Expression *, EItemInfo> >();

		if ($3) {
			EItemInfo info = { true, @3 };
			$$->push_back(std::make_pair($3, info));
		}
	}
	| lterm_items_inner sep rterm_no_side_effect %dprec 1
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<std::pair<Expression *, EItemInfo> >();

		if ($3) {
			EItemInfo info = { false, @3 };
			$$->push_back(std::make_pair($3, info));
		}
	}
	;

library: T_LIBRARY T_STRING
	{
		context->HandleLibrary($2);
		free($2);
	}
	;

identifier: T_IDENTIFIER
	| T_STRING
	;

type: T_TYPE identifier
	{
		String name = String($2);
		free($2);

		context->m_Type = ConfigType::GetByName(name);

		if (!context->m_Type) {
			context->m_Type = new ConfigType(name, @$);
			context->m_Type->Register();
		}
	}
	type_inherits_specifier typerulelist
	{
		TypeRuleList::Ptr ruleList = *$5;
		delete $5;

		context->m_Type->GetRuleList()->AddRules(ruleList);
		context->m_Type->GetRuleList()->AddRequires(ruleList);

		BOOST_FOREACH(const String& validator, ruleList->GetValidators()) {
			context->m_Type->GetRuleList()->AddValidator(validator);
		}
	}
	;

typerulelist: '{'
	{
		context->m_OpenBraces++;
		context->m_RuleLists.push(new TypeRuleList());
	}
	typerules
	'}'
	{
		context->m_OpenBraces--;
		$$ = new Value(context->m_RuleLists.top());
		context->m_RuleLists.pop();
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
		context->m_RuleLists.top()->AddRequire($2);
		free($2);
	}
	| T_VALIDATOR T_STRING
	{
		context->m_RuleLists.top()->AddValidator($2);
		free($2);
	}
	| T_ATTRIBUTE type T_STRING
	{
		TypeRule rule($2, String(), $3, TypeRuleList::Ptr(), @$);
		free($3);

		context->m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE T_TYPE_NAME '(' identifier ')' T_STRING
	{
		TypeRule rule($2, $4, $6, TypeRuleList::Ptr(), @$);
		free($4);
		free($6);

		context->m_RuleLists.top()->AddRule(rule);
	}
	| T_ATTRIBUTE type T_STRING typerulelist
	{
		TypeRule rule($2, String(), $3, *$4, @$);
		free($3);
		delete $4;
		context->m_RuleLists.top()->AddRule(rule);
	}
	;

type_inherits_specifier: /* empty */
	| T_INHERITS identifier
	{
		context->m_Type->SetParent($2);
		free($2);
	}
	;

type: T_TYPE_DICTIONARY
	| T_TYPE_ARRAY
	| T_TYPE_NUMBER
	| T_TYPE_STRING
	| T_TYPE_SCALAR
	| T_TYPE_ANY
	| T_TYPE_FUNCTION
	| T_TYPE_NAME
	;

object:
	{
		context->m_ObjectAssign.push(true);
		context->m_SeenAssign.push(false);
		context->m_Assign.push(NULL);
		context->m_Ignore.push(NULL);
	}
	object_declaration identifier optional_rterm use_specifier rterm_scope_require_side_effect
	{
		context->m_ObjectAssign.pop();

		bool abstract = $2;

		String type = $3;
		free($3);

		$6->MakeInline();

		bool seen_assign = context->m_SeenAssign.top();
		context->m_SeenAssign.pop();

		Expression *ignore = context->m_Ignore.top();
		context->m_Ignore.pop();

		Expression *assign = context->m_Assign.top();
		context->m_Assign.pop();

		Expression *filter = NULL;

		if (seen_assign) {
			if (!ObjectRule::IsValidSourceType(type))
				BOOST_THROW_EXCEPTION(ScriptError("object rule 'assign' cannot be used for type '" + type + "'", DebugInfoRange(@2, @3)));

			if (ignore) {
				Expression *rex = new LogicalNegateExpression(ignore, DebugInfoRange(@2, @5));

				filter = new LogicalAndExpression(assign, rex, DebugInfoRange(@2, @5));
			} else
				filter = assign;
		}

		$$ = new ObjectExpression(abstract, type, $4, filter, context->GetZone(), $5, $6, DebugInfoRange(@2, @5));
	}
	;

object_declaration: T_OBJECT
	{
		$$ = false;
	}
	| T_TEMPLATE
	{
		$$ = true;
	}
	;

identifier_items: /* empty */
	{
		$$ = new std::vector<String>();
	}
	| identifier_items_inner
	| identifier_items_inner ','
	;

identifier_items_inner: identifier
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

combined_set_op: T_SET
	| T_SET_ADD
	| T_SET_SUBTRACT
	| T_SET_MULTIPLY
	| T_SET_DIVIDE
	| T_SET_MODULO
	| T_SET_XOR
	| T_SET_BINARY_AND
	| T_SET_BINARY_OR
	;

lterm: type
	{
		$$ = MakeLiteral(); // ASTify this
	}
	| library
	{
		$$ = MakeLiteral(); // ASTify this
	}
	| rterm combined_set_op rterm
	{
		$$ = new SetExpression($1, $2, $3, @$);
	}
	| T_INCLUDE T_STRING
	{
		$$ = context->HandleInclude($2, false, @$);
		free($2);
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		$$ = context->HandleInclude($2, true, @$);
		free($2);
	}
	| T_INCLUDE_RECURSIVE T_STRING
	{
		$$ = context->HandleIncludeRecursive($2, "*.conf", @$);
		free($2);
	}
	| T_INCLUDE_RECURSIVE T_STRING ',' T_STRING
	{
		$$ = context->HandleIncludeRecursive($2, $4, @$);
		free($2);
		free($4);
	}
	| T_IMPORT rterm
	{
		$$ = new ImportExpression($2, @$);
	}
	| T_ASSIGN T_WHERE rterm
	{
		if ((context->m_Apply.empty() || !context->m_Apply.top()) && (context->m_ObjectAssign.empty() || !context->m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ScriptError("'assign' keyword not valid in this context.", @$));

		context->m_SeenAssign.top() = true;

		if (context->m_Assign.top())
			context->m_Assign.top() = new LogicalOrExpression(context->m_Assign.top(), $3, @$);
		else
			context->m_Assign.top() = $3;

		$$ = MakeLiteral();
	}
	| T_IGNORE T_WHERE rterm
	{
		if ((context->m_Apply.empty() || !context->m_Apply.top()) && (context->m_ObjectAssign.empty() || !context->m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ScriptError("'ignore' keyword not valid in this context.", @$));

		if (context->m_Ignore.top())
			context->m_Ignore.top() = new LogicalOrExpression(context->m_Ignore.top(), $3, @$);
		else
			context->m_Ignore.top() = $3;

		$$ = MakeLiteral();
	}
	| T_RETURN optional_rterm
	{
		$$ = new ReturnExpression($2, @$);
	}
	| T_BREAK
	{
		$$ = new BreakExpression(@$);
	}
	| T_CONTINUE
	{
		$$ = new ContinueExpression(@$);
	}
	| apply
	| object
	| T_FOR '(' identifier T_FOLLOWS identifier T_IN rterm ')' rterm_scope_require_side_effect
	{
		$9->MakeInline();

		$$ = new ForExpression($3, $5, $7, $9, @$);
		free($3);
		free($5);
	}
	| T_FOR '(' identifier T_IN rterm ')' rterm_scope_require_side_effect
	{
		$7->MakeInline();

		$$ = new ForExpression($3, "", $5, $7, @$);
		free($3);
	}
	| T_FUNCTION identifier '(' identifier_items ')' use_specifier rterm_scope
	{
		$7->MakeInline();

		FunctionExpression *fexpr = new FunctionExpression(*$4, $6, $7, @$);
		delete $4;

		$$ = new SetExpression(MakeIndexer(ScopeCurrent, $2), OpSetLiteral, fexpr, @$);
		free($2);
	}
	| T_CONST T_IDENTIFIER T_SET rterm
	{
		$$ = new SetExpression(MakeIndexer(ScopeGlobal, $2), OpSetLiteral, $4);
		free($2);
	}
	| T_VAR rterm
	{
		Expression *expr = $2;
		BindToScope(expr, ScopeLocal);
		$$ = new SetExpression(expr, OpSetLiteral, MakeLiteral(), @$);
	}
	| T_VAR rterm combined_set_op rterm
	{
		Expression *expr = $2;
		BindToScope(expr, ScopeLocal);
		$$ = new SetExpression(expr, $3, $4, @$);
	}
	| T_WHILE '(' rterm ')' rterm_scope
	{
		$5->MakeInline();

		$$ = new WhileExpression($3, $5, @$);
	}
	| rterm_side_effect
	;
	
rterm_items: /* empty */
	{
		$$ = new std::vector<Expression *>();
	}
	| rterm_items_inner
	| rterm_items_inner ','
	| rterm_items_inner ',' newlines
	| rterm_items_inner newlines
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

rterm_array: '['
	{
		context->m_OpenBraces++;
	}
	newlines rterm_items ']'
	{
		context->m_OpenBraces--;
		$$ = new ArrayExpression(*$4, @$);
		delete $4;
	}
	| '['
	{
		context->m_OpenBraces++;
	}
	rterm_items ']'
	{
		$$ = new ArrayExpression(*$3, @$);
		delete $3;
	}
	;

rterm_scope_require_side_effect: '{'
	{
		context->m_OpenBraces++;
	}
	statements '}'
	{
		context->m_OpenBraces--;
		std::vector<Expression *> dlist;
		typedef std::pair<Expression *, EItemInfo> EListItem;
		int num = 0;
		BOOST_FOREACH(const EListItem& litem, *$3) {
			if (!litem.second.SideEffect)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(litem.first);
			num++;
		}
		delete $3;
		$$ = new DictExpression(dlist, @$);
	}
	;

rterm_scope: '{'
	{
		context->m_OpenBraces++;
	}
	statements '}'
	{
		context->m_OpenBraces--;
		std::vector<Expression *> dlist;
		typedef std::pair<Expression *, EItemInfo> EListItem;
		int num = 0;
		BOOST_FOREACH(const EListItem& litem, *$3) {
			if (!litem.second.SideEffect && num != $3->size() - 1)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(litem.first);
			num++;
		}
		delete $3;
		$$ = new DictExpression(dlist, @$);
	}
	;

else_if_branch: T_ELSE T_IF '(' rterm ')' rterm_scope
	{
		$6->MakeInline();

		$$ = new std::pair<Expression *, Expression *>($4, $6);
	}
	;

else_if_branches: /* empty */
	{
		$$ = new std::vector<std::pair<Expression *, Expression *> >();
	}
	| else_if_branches else_if_branch
	{
		$$ = $1;
		$$->push_back(*$2);
		delete $2;
	}
	;

rterm_side_effect: rterm '(' rterm_items ')'
	{
		$$ = new FunctionCallExpression($1, *$3, @$);
		delete $3;
	}
	| identifier T_FOLLOWS rterm
	{
		DictExpression *aexpr = dynamic_cast<DictExpression *>($3);
		if (aexpr)
			aexpr->MakeInline();

		std::vector<String> args;
		args.push_back($1);
		free($1);

		$$ = new FunctionExpression(args, new std::map<String, Expression *>(), $3, @$);
	}
	| '(' identifier_items ')' T_FOLLOWS rterm
	{
		DictExpression *aexpr = dynamic_cast<DictExpression *>($5);
		if (aexpr)
			aexpr->MakeInline();

		$$ = new FunctionExpression(*$2, new std::map<String, Expression *>(), $5, @$);
		delete $2;
	}
	| T_IF '(' rterm ')' rterm_scope else_if_branches
	{
		$5->MakeInline();

		std::vector<std::pair<Expression *, Expression *> > ebranches = *$6;
		delete $6;

		Expression *afalse = NULL;

		for (int i = ebranches.size() - 1; i >= 0; i--) {
			const std::pair<Expression *, Expression *>& ebranch = ebranches[i];
			afalse = new ConditionalExpression(ebranch.first, ebranch.second, afalse, @6);
		}

		$$ = new ConditionalExpression($3, $5, afalse, @$);
	}
	| T_IF '(' rterm ')' rterm_scope else_if_branches T_ELSE rterm_scope
	{
		$5->MakeInline();

		std::vector<std::pair<Expression *, Expression *> > ebranches = *$6;
		delete $6;

		$8->MakeInline();

		Expression *afalse = $8;

		for (int i = ebranches.size() - 1; i >= 0; i--) {
			const std::pair<Expression *, Expression *>& ebranch = ebranches[i];
			afalse = new ConditionalExpression(ebranch.first, ebranch.second, afalse, @6);
		}

		$$ = new ConditionalExpression($3, $5, afalse, @$);
	}
	;

rterm_no_side_effect: T_STRING
	{
		$$ = MakeLiteral($1);
		free($1);
	}
	| T_NUMBER
	{
		$$ = MakeLiteral($1);
	}
	| T_BOOLEAN
	{
		$$ = MakeLiteral($1);
	}
	| T_NULL
	{
		$$ = MakeLiteral();
	}
	| rterm '.' T_IDENTIFIER %dprec 2
	{
		$$ = new IndexerExpression($1, MakeLiteral($3), @$);
		free($3);
	}
	| rterm '[' rterm ']'
	{
		$$ = new IndexerExpression($1, $3, @$);
	}
	| T_IDENTIFIER
	{
		$$ = new VariableExpression($1, @1);
		free($1);
	}
	| '!' rterm
	{
		$$ = new LogicalNegateExpression($2, @$);
	}
	| '~' rterm
	{
		$$ = new NegateExpression($2, @$);
	}
	| T_PLUS rterm %prec UNARY_PLUS
	{
		$$ = $2;
	}
	| T_MINUS rterm %prec UNARY_MINUS
	{
		$$ = new SubtractExpression(MakeLiteral(0), $2, @$);
	}
	| T_THIS
	{
		$$ = new GetScopeExpression(ScopeThis);
	}
	| T_GLOBALS
	{
		$$ = new GetScopeExpression(ScopeGlobal);
	}
	| T_LOCALS
	{
		$$ = new GetScopeExpression(ScopeLocal);
	}
	| rterm_array
	| rterm_scope_require_side_effect
	{
		Expression *expr = $1;
		BindToScope(expr, ScopeCurrent);
		$$ = expr;
	}
	| '('
	{
		context->m_OpenBraces++;
	}
	rterm ')'
	{
		context->m_OpenBraces--;
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
	| rterm T_MODULO rterm { MakeRBinaryOp<ModuloExpression>(&$$, $1, $3, @1, @3); }
	| rterm T_XOR rterm { MakeRBinaryOp<XorExpression>(&$$, $1, $3, @1, @3); }
	| T_FUNCTION '(' identifier_items ')' use_specifier rterm_scope
	{
		$6->MakeInline();

		$$ = new FunctionExpression(*$3, $5, $6, @$);
		delete $3;
	}
	| T_NULLARY_LAMBDA_BEGIN statements T_NULLARY_LAMBDA_END
	{
		std::vector<Expression *> dlist;
		typedef std::pair<Expression *, EItemInfo> EListItem;
		int num = 0;
		BOOST_FOREACH(const EListItem& litem, *$2) {
			if (!litem.second.SideEffect && num != $2->size() - 1)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(litem.first);
			num++;
		}
		delete $2;
		DictExpression *aexpr = new DictExpression(dlist, @$);
		aexpr->MakeInline();

		$$ = new FunctionExpression(std::vector<String>(), NULL, aexpr, @$);
	}
	;

rterm: rterm_side_effect
	| rterm_no_side_effect
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

use_specifier: /* empty */
	{
		$$ = NULL;
	}
	| T_USE '(' use_specifier_items ')'
	{
		$$ = $3;
	}
	;

use_specifier_items: use_specifier_item
	{
		$$ = new std::map<String, Expression *>();
		$$->insert(*$1);
		delete $1;
	}
	| use_specifier_items ',' use_specifier_item
	{
		$$ = $1;
		$$->insert(*$3);
		delete $3;
	}
	;

use_specifier_item: identifier
	{
		$$ = new std::pair<String, Expression *>($1, new VariableExpression($1, @1));
	}
	| identifier T_SET rterm
	{
		$$ = new std::pair<String, Expression *>($1, $3);
	}
	;

apply_for_specifier: /* empty */
	| T_FOR '(' identifier T_FOLLOWS identifier T_IN rterm ')'
	{
		context->m_FKVar.top() = $3;
		free($3);

		context->m_FVVar.top() = $5;
		free($5);

		context->m_FTerm.top() = $7;
	}
	| T_FOR '(' identifier T_IN rterm ')'
	{
		context->m_FKVar.top() = $3;
		free($3);

		context->m_FVVar.top() = "";

		context->m_FTerm.top() = $5;
	}
	;

optional_rterm: /* empty */
	{
		$$ = MakeLiteral();
	}
	| rterm
	;

apply:
	{
		context->m_Apply.push(true);
		context->m_SeenAssign.push(false);
		context->m_Assign.push(NULL);
		context->m_Ignore.push(NULL);
		context->m_FKVar.push("");
		context->m_FVVar.push("");
		context->m_FTerm.push(NULL);
	}
	T_APPLY identifier optional_rterm apply_for_specifier target_type_specifier use_specifier rterm_scope_require_side_effect
	{
		context->m_Apply.pop();

		String type = $3;
		free($3);
		String target = $6;
		free($6);

		if (!ApplyRule::IsValidSourceType(type))
			BOOST_THROW_EXCEPTION(ScriptError("'apply' cannot be used with type '" + type + "'", DebugInfoRange(@2, @3)));

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

				BOOST_THROW_EXCEPTION(ScriptError("'apply' target type is ambiguous (can be one of " + typeNames + "): use 'to' to specify a type", DebugInfoRange(@2, @3)));
			} else
				BOOST_THROW_EXCEPTION(ScriptError("'apply' target type '" + target + "' is invalid", DebugInfoRange(@2, @5)));
		}

		$8->MakeInline();

		bool seen_assign = context->m_SeenAssign.top();
		context->m_SeenAssign.pop();

		// assign && !ignore
		if (!seen_assign && !context->m_FTerm.top())
			BOOST_THROW_EXCEPTION(ScriptError("'apply' is missing 'assign'/'for'", DebugInfoRange(@2, @3)));

		Expression *ignore = context->m_Ignore.top();
		context->m_Ignore.pop();

		Expression *assign;

		if (!seen_assign)
			assign = MakeLiteral(true);
		else
			assign = context->m_Assign.top();

		context->m_Assign.pop();

		Expression *filter;

		if (ignore) {
			Expression *rex = new LogicalNegateExpression(ignore, DebugInfoRange(@2, @5));

			filter = new LogicalAndExpression(assign, rex, DebugInfoRange(@2, @5));
		} else
			filter = assign;

		String fkvar = context->m_FKVar.top();
		context->m_FKVar.pop();

		String fvvar = context->m_FVVar.top();
		context->m_FVVar.pop();

		Expression *fterm = context->m_FTerm.top();
		context->m_FTerm.pop();

		$$ = new ApplyExpression(type, target, $4, filter, fkvar, fvvar, fterm, $7, $8, DebugInfoRange(@2, @7));
	}
	;

newlines: T_NEWLINE
	| T_NEWLINE newlines
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
