%{
#define YYDEBUG 1
 
/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "base/value.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/configtype.hpp"
#include "base/exception.hpp"
#include <sstream>
#include <stack>

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

#define YYINITDEPTH 10000

using namespace icinga;

template<typename T>
static void MakeRBinaryOp(Expression** result, Expression *left, Expression *right, const DebugInfo& diLeft, const DebugInfo& diRight)
{
	*result = new T(std::unique_ptr<Expression>(left), std::unique_ptr<Expression>(right), DebugInfoRange(diLeft, diRight));
}

%}

%pure-parser

%locations
%defines
%error-verbose
%glr-parser

%parse-param { std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> > *llist }
%parse-param { ConfigCompiler *context }
%lex-param { void *scanner }

%union {
	String *text;
	double num;
	bool boolean;
	icinga::Expression *expr;
	icinga::DictExpression *dexpr;
	CombinedSetOp csop;
	std::vector<String> *slist;
	std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> > *llist;
	std::vector<std::unique_ptr<Expression> > *elist;
	std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression> > > *ebranchlist;
	std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression> > *ebranch;
	std::pair<String, std::unique_ptr<Expression> > *cvitem;
	std::map<String, std::unique_ptr<Expression> > *cvlist;
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
%token T_DEFAULT "default (T_DEFAULT)"
%token T_IGNORE_ON_ERROR "ignore_on_error (T_IGNORE_ON_ERROR)"
%token T_CURRENT_FILENAME "current_filename (T_CURRENT_FILENAME)"
%token T_CURRENT_LINE "current_line (T_CURRENT_LINE)"
%token T_DEBUGGER "debugger (T_DEBUGGER)"
%token T_USE "use (T_USE)"
%token T_USING "__using (T_USING)"
%token T_OBJECT "object (T_OBJECT)"
%token T_TEMPLATE "template (T_TEMPLATE)"
%token T_INCLUDE "include (T_INCLUDE)"
%token T_INCLUDE_RECURSIVE "include_recursive (T_INCLUDE_RECURSIVE)"
%token T_INCLUDE_ZONES "include_zones (T_INCLUDE_ZONES)"
%token T_LIBRARY "library (T_LIBRARY)"
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
%token T_THROW "throw (T_THROW)"
%token T_TRY "try (T_TRY)"
%token T_EXCEPT "except (T_EXCEPT)"
%token T_FOLLOWS "=> (T_FOLLOWS)"
%token T_NULLARY_LAMBDA_BEGIN "{{ (T_NULLARY_LAMBDA_BEGIN)"
%token T_NULLARY_LAMBDA_END "}} (T_NULLARY_LAMBDA_END)"

%type <text> identifier
%type <elist> rterm_items
%type <elist> rterm_items_inner
%type <slist> identifier_items
%type <slist> identifier_items_inner
%type <csop> combined_set_op
%type <llist> statements
%type <llist> lterm_items
%type <llist> lterm_items_inner
%type <expr> rterm
%type <expr> rterm_array
%type <dexpr> rterm_dict
%type <dexpr> rterm_scope_require_side_effect
%type <dexpr> rterm_scope
%type <ebranchlist> else_if_branches
%type <ebranch> else_if_branch
%type <expr> rterm_side_effect
%type <expr> rterm_no_side_effect
%type <expr> rterm_no_side_effect_no_dict
%type <expr> lterm
%type <expr> object
%type <expr> apply
%type <expr> optional_rterm
%type <text> target_type_specifier
%type <boolean> default_specifier
%type <boolean> ignore_specifier
%type <cvlist> use_specifier
%type <cvlist> use_specifier_items
%type <cvitem> use_specifier_item
%type <num> object_declaration

%right T_FOLLOWS
%right T_INCLUDE T_INCLUDE_RECURSIVE T_INCLUDE_ZONES T_OBJECT T_TEMPLATE T_APPLY T_IMPORT T_ASSIGN T_IGNORE T_WHERE
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

void yyerror(const YYLTYPE *locp, std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> > *, ConfigCompiler *context, const char *err)
{
	bool incomplete = context && context->m_Eof && (context->m_OpenBraces > 0);
	BOOST_THROW_EXCEPTION(ScriptError(err, *locp, incomplete));
}

int yyparse(std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> > *llist, ConfigCompiler *context);

static void BeginFlowControlBlock(ConfigCompiler *compiler, int allowedTypes, bool inherit)
{
	if (inherit)
		allowedTypes |= compiler->m_FlowControlInfo.top();

	compiler->m_FlowControlInfo.push(allowedTypes);
}

static void EndFlowControlBlock(ConfigCompiler *compiler)
{
	compiler->m_FlowControlInfo.pop();
}

static void UseFlowControl(ConfigCompiler *compiler, FlowControlType type, const CompilerDebugInfo& location)
{
	int fci = compiler->m_FlowControlInfo.top();

	if ((type & fci) != type)
		BOOST_THROW_EXCEPTION(ScriptError("Invalid flow control statement.", location));
}

std::unique_ptr<Expression> ConfigCompiler::Compile()
{
	std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> > llist;

	//yydebug = 1;

	m_IgnoreNewlines.push(false);
	BeginFlowControlBlock(this, 0, false);

	if (yyparse(&llist, this) != 0)
		return NULL;

	EndFlowControlBlock(this);
	m_IgnoreNewlines.pop();

	std::vector<std::unique_ptr<Expression> > dlist;
	decltype(llist.size()) num = 0;
	for (auto& litem : llist) {
		if (!litem.second.SideEffect && num != llist.size() - 1) {
			yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
		}
		dlist.push_back(std::move(litem.first));
		num++;
	}

	std::unique_ptr<DictExpression> expr{new DictExpression(std::move(dlist))};
	expr->MakeInline();
	return std::move(expr);
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
		$$ = new std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> >();
	}
	| lterm_items_inner
	| lterm_items_inner sep
	;

lterm_items_inner: lterm %dprec 2
	{
		$$ = new std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> >();
		EItemInfo info = { true, @1 };
		$$->emplace_back(std::unique_ptr<Expression>($1), info);
	}
	| rterm_no_side_effect
	{
		$$ = new std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> >();
		EItemInfo info = { false, @1 };
		$$->emplace_back(std::unique_ptr<Expression>($1), info);
	}
	| lterm_items_inner sep lterm %dprec 1
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> >();

		if ($3) {
			EItemInfo info = { true, @3 };
			$$->emplace_back(std::unique_ptr<Expression>($3), info);
		}
	}
	| lterm_items_inner sep rterm_no_side_effect %dprec 1
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<std::pair<std::unique_ptr<Expression>, EItemInfo> >();

		if ($3) {
			EItemInfo info = { false, @3 };
			$$->emplace_back(std::unique_ptr<Expression>($3), info);
		}
	}
	;

identifier: T_IDENTIFIER
	| T_STRING
	;

object:
	{
		context->m_ObjectAssign.push(true);
		context->m_SeenAssign.push(false);
		context->m_SeenIgnore.push(false);
		context->m_Assign.push(0);
		context->m_Ignore.push(0);
	}
	object_declaration rterm optional_rterm use_specifier default_specifier ignore_specifier
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope_require_side_effect
	{
		EndFlowControlBlock(context);

		context->m_ObjectAssign.pop();

		bool abstract = $2;
		bool defaultTmpl = $6;

		if (!abstract && defaultTmpl)
			BOOST_THROW_EXCEPTION(ScriptError("'default' keyword is invalid for object definitions", DebugInfoRange(@2, @4)));

		bool seen_assign = context->m_SeenAssign.top();
		context->m_SeenAssign.pop();

		bool seen_ignore = context->m_SeenIgnore.top();
		context->m_SeenIgnore.pop();

		std::unique_ptr<Expression> ignore{std::move(context->m_Ignore.top())};
		context->m_Ignore.pop();

		std::unique_ptr<Expression> assign{std::move(context->m_Assign.top())};
		context->m_Assign.pop();

		std::unique_ptr<Expression> filter;

		if (seen_assign) {
			if (ignore) {
				std::unique_ptr<Expression> rex{new LogicalNegateExpression(std::move(ignore), DebugInfoRange(@2, @5))};

				filter.reset(new LogicalAndExpression(std::move(assign), std::move(rex), DebugInfoRange(@2, @5)));
			} else
				filter.swap(assign);
		} else if (seen_ignore) {
			BOOST_THROW_EXCEPTION(ScriptError("object rule 'ignore where' cannot be used without 'assign where'", DebugInfoRange(@2, @4)));
		}

		$$ = new ObjectExpression(abstract, std::unique_ptr<Expression>($3), std::unique_ptr<Expression>($4),
			std::move(filter), context->GetZone(), context->GetPackage(), std::move(*$5), $6, $7,
			std::unique_ptr<Expression>($9), DebugInfoRange(@2, @7));
		delete $5;
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
		$$->push_back(*$1);
		delete $1;
	}
	| identifier_items_inner ',' identifier
	{
		if ($1)
			$$ = $1;
		else
			$$ = new std::vector<String>();

		$$->push_back(*$3);
		delete $3;
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

lterm: T_LIBRARY rterm
	{
		$$ = new LibraryExpression(std::unique_ptr<Expression>($2), @$);
	}
	| rterm combined_set_op rterm
	{
		$$ = new SetExpression(std::unique_ptr<Expression>($1), $2, std::unique_ptr<Expression>($3), @$);
	}
	| T_INCLUDE rterm
	{
		$$ = new IncludeExpression(Utility::DirName(context->GetPath()), std::unique_ptr<Expression>($2), NULL, NULL, IncludeRegular, false, context->GetZone(), context->GetPackage(), @$);
	}
	| T_INCLUDE T_STRING_ANGLE
	{
		$$ = new IncludeExpression(Utility::DirName(context->GetPath()), MakeLiteral(*$2), NULL, NULL, IncludeRegular, true, context->GetZone(), context->GetPackage(), @$);
		delete $2;
	}
	| T_INCLUDE_RECURSIVE rterm
	{
		$$ = new IncludeExpression(Utility::DirName(context->GetPath()), std::unique_ptr<Expression>($2), MakeLiteral("*.conf"), NULL, IncludeRecursive, false, context->GetZone(), context->GetPackage(), @$);
	}
	| T_INCLUDE_RECURSIVE rterm ',' rterm
	{
		$$ = new IncludeExpression(Utility::DirName(context->GetPath()), std::unique_ptr<Expression>($2), std::unique_ptr<Expression>($4), NULL, IncludeRecursive, false, context->GetZone(), context->GetPackage(), @$);
	}
	| T_INCLUDE_ZONES rterm ',' rterm
	{
		$$ = new IncludeExpression(Utility::DirName(context->GetPath()), std::unique_ptr<Expression>($4), MakeLiteral("*.conf"), std::unique_ptr<Expression>($2), IncludeZones, false, context->GetZone(), context->GetPackage(), @$);
	}
	| T_INCLUDE_ZONES rterm ',' rterm ',' rterm
	{
		$$ = new IncludeExpression(Utility::DirName(context->GetPath()), std::unique_ptr<Expression>($4), std::unique_ptr<Expression>($6), std::unique_ptr<Expression>($2), IncludeZones, false, context->GetZone(), context->GetPackage(), @$);
	}
	| T_IMPORT rterm
	{
		$$ = new ImportExpression(std::unique_ptr<Expression>($2), @$);
	}
	| T_ASSIGN T_WHERE
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope %dprec 2
	{
		EndFlowControlBlock(context);

		if ((context->m_Apply.empty() || !context->m_Apply.top()) && (context->m_ObjectAssign.empty() || !context->m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ScriptError("'assign' keyword not valid in this context.", @$));

		context->m_SeenAssign.top() = true;

		if (context->m_Assign.top())
			context->m_Assign.top() = new LogicalOrExpression(std::unique_ptr<Expression>(context->m_Assign.top()), std::unique_ptr<Expression>($4), @$);
		else
			context->m_Assign.top() = $4;

		$$ = MakeLiteralRaw();
	}
	| T_ASSIGN T_WHERE rterm %dprec 1
	{
		ASSERT(!dynamic_cast<DictExpression *>($3));

		if ((context->m_Apply.empty() || !context->m_Apply.top()) && (context->m_ObjectAssign.empty() || !context->m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ScriptError("'assign' keyword not valid in this context.", @$));

		context->m_SeenAssign.top() = true;

		if (context->m_Assign.top())
			context->m_Assign.top() = new LogicalOrExpression(std::unique_ptr<Expression>(context->m_Assign.top()), std::unique_ptr<Expression>($3), @$);
		else
			context->m_Assign.top() = $3;

		$$ = MakeLiteralRaw();
	}
	| T_IGNORE T_WHERE
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope %dprec 2
	{
		EndFlowControlBlock(context);

		if ((context->m_Apply.empty() || !context->m_Apply.top()) && (context->m_ObjectAssign.empty() || !context->m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ScriptError("'ignore' keyword not valid in this context.", @$));

		context->m_SeenIgnore.top() = true;

		if (context->m_Ignore.top())
			context->m_Ignore.top() = new LogicalOrExpression(std::unique_ptr<Expression>(context->m_Ignore.top()), std::unique_ptr<Expression>($4), @$);
		else
			context->m_Ignore.top() = $4;

		$$ = MakeLiteralRaw();
	}
	| T_IGNORE T_WHERE rterm %dprec 1
	{
		ASSERT(!dynamic_cast<DictExpression *>($3));

		if ((context->m_Apply.empty() || !context->m_Apply.top()) && (context->m_ObjectAssign.empty() || !context->m_ObjectAssign.top()))
			BOOST_THROW_EXCEPTION(ScriptError("'ignore' keyword not valid in this context.", @$));

		context->m_SeenIgnore.top() = true;

		if (context->m_Ignore.top())
			context->m_Ignore.top() = new LogicalOrExpression(std::unique_ptr<Expression>(context->m_Ignore.top()), std::unique_ptr<Expression>($3), @$);
		else
			context->m_Ignore.top() = $3;

		$$ = MakeLiteralRaw();
	}
	| T_RETURN optional_rterm
	{
		UseFlowControl(context, FlowControlReturn, @$);
		$$ = new ReturnExpression(std::unique_ptr<Expression>($2), @$);
	}
	| T_BREAK
	{
		UseFlowControl(context, FlowControlBreak, @$);
		$$ = new BreakExpression(@$);
	}
	| T_CONTINUE
	{
		UseFlowControl(context, FlowControlContinue, @$);
		$$ = new ContinueExpression(@$);
	}
	| T_DEBUGGER
	{
		$$ = new BreakpointExpression(@$);
	}
	| T_USING rterm
	{
		$$ = new UsingExpression(std::unique_ptr<Expression>($2), @$);
	}
	| apply
	| object
	| T_FOR '(' identifier T_FOLLOWS identifier T_IN rterm ')'
	{
		BeginFlowControlBlock(context, FlowControlContinue | FlowControlBreak, true);
	}
	rterm_scope_require_side_effect
	{
		EndFlowControlBlock(context);

		$$ = new ForExpression(*$3, *$5, std::unique_ptr<Expression>($7), std::unique_ptr<Expression>($10), @$);
		delete $3;
		delete $5;
	}
	| T_FOR '(' identifier T_IN rterm ')'
	{
		BeginFlowControlBlock(context, FlowControlContinue | FlowControlBreak, true);
	}
	rterm_scope_require_side_effect
	{
		EndFlowControlBlock(context);

		$$ = new ForExpression(*$3, "", std::unique_ptr<Expression>($5), std::unique_ptr<Expression>($8), @$);
		delete $3;
	}
	| T_FUNCTION identifier '(' identifier_items ')' use_specifier
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope
	{
		EndFlowControlBlock(context);

		std::unique_ptr<FunctionExpression> fexpr{new FunctionExpression(*$2, *$4, std::move(*$6), std::unique_ptr<Expression>($8), @$)};
		delete $4;
		delete $6;

		$$ = new SetExpression(MakeIndexer(ScopeThis, *$2), OpSetLiteral, std::move(fexpr), @$);
		delete $2;
	}
	| T_CONST T_IDENTIFIER T_SET rterm
	{
		$$ = new SetExpression(MakeIndexer(ScopeGlobal, *$2), OpSetLiteral, std::unique_ptr<Expression>($4));
		delete $2;
	}
	| T_VAR rterm
	{
		std::unique_ptr<Expression> expr{$2};
		BindToScope(expr, ScopeLocal);
		$$ = new SetExpression(std::move(expr), OpSetLiteral, MakeLiteral(), @$);
	}
	| T_VAR rterm combined_set_op rterm
	{
		std::unique_ptr<Expression> expr{$2};
		BindToScope(expr, ScopeLocal);
		$$ = new SetExpression(std::move(expr), $3, std::unique_ptr<Expression>($4), @$);
	}
	| T_WHILE '(' rterm ')'
	{
		BeginFlowControlBlock(context, FlowControlContinue | FlowControlBreak, true);
	}
	rterm_scope
	{
		EndFlowControlBlock(context);

		$$ = new WhileExpression(std::unique_ptr<Expression>($3), std::unique_ptr<Expression>($6), @$);
	}
	| T_THROW rterm
	{
		$$ = new ThrowExpression(std::unique_ptr<Expression>($2), false, @$);
	}
	| T_TRY rterm_scope T_EXCEPT rterm_scope
	{
		$$ = new TryExceptExpression(std::unique_ptr<Expression>($2), std::unique_ptr<Expression>($4), @$);
	}
	| rterm_side_effect
	;

rterm_items: /* empty */
	{
		$$ = new std::vector<std::unique_ptr<Expression> >();
	}
	| rterm_items_inner
	| rterm_items_inner ','
	| rterm_items_inner ',' newlines
	| rterm_items_inner newlines
	;

rterm_items_inner: rterm
	{
		$$ = new std::vector<std::unique_ptr<Expression> >();
		$$->emplace_back($1);
	}
	| rterm_items_inner arraysep rterm
	{
		$$ = $1;
		$$->emplace_back($3);
	}
	;

rterm_array: '['
	{
		context->m_OpenBraces++;
	}
	newlines rterm_items ']'
	{
		context->m_OpenBraces--;
		$$ = new ArrayExpression(std::move(*$4), @$);
		delete $4;
	}
	| '['
	{
		context->m_OpenBraces++;
	}
	rterm_items ']'
	{
		context->m_OpenBraces--;
		$$ = new ArrayExpression(std::move(*$3), @$);
		delete $3;
	}
	;

rterm_dict: '{'
	{
		BeginFlowControlBlock(context, 0, false);
		context->m_IgnoreNewlines.push(false);
		context->m_OpenBraces++;
	}
	statements '}'
	{
		EndFlowControlBlock(context);
		context->m_OpenBraces--;
		context->m_IgnoreNewlines.pop();
		std::vector<std::unique_ptr<Expression> > dlist;
		for (auto& litem : *$3) {
			if (!litem.second.SideEffect)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(std::move(litem.first));
		}
		delete $3;
		$$ = new DictExpression(std::move(dlist), @$);
	}
	;

rterm_scope_require_side_effect: '{'
	{
		context->m_IgnoreNewlines.push(false);
		context->m_OpenBraces++;
	}
	statements '}'
	{
		context->m_OpenBraces--;
		context->m_IgnoreNewlines.pop();
		std::vector<std::unique_ptr<Expression> > dlist;
		for (auto& litem : *$3) {
			if (!litem.second.SideEffect)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(std::move(litem.first));
		}
		delete $3;
		$$ = new DictExpression(std::move(dlist), @$);
		$$->MakeInline();
	}
	;

rterm_scope: '{'
	{
		context->m_IgnoreNewlines.push(false);
		context->m_OpenBraces++;
	}
	statements '}'
	{
		context->m_OpenBraces--;
		context->m_IgnoreNewlines.pop();
		std::vector<std::unique_ptr<Expression> > dlist;
		decltype($3->size()) num = 0;
		for (auto& litem : *$3) {
			if (!litem.second.SideEffect && num != $3->size() - 1)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(std::move(litem.first));
			num++;
		}
		delete $3;
		$$ = new DictExpression(std::move(dlist), @$);
		$$->MakeInline();
	}
	;

else_if_branch: T_ELSE T_IF '(' rterm ')' rterm_scope
	{
		$$ = new std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression> >(std::unique_ptr<Expression>($4), std::unique_ptr<Expression>($6));
	}
	;

else_if_branches: /* empty */
	{
		$$ = new std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression> > >();
	}
	| else_if_branches else_if_branch
	{
		$$ = $1;
		$$->push_back(std::move(*$2));
		delete $2;
	}
	;

rterm_side_effect: rterm '(' rterm_items ')'
	{
		$$ = new FunctionCallExpression(std::unique_ptr<Expression>($1), std::move(*$3), @$);
		delete $3;
	}
	| T_IF '(' rterm ')' rterm_scope else_if_branches
	{
		std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression> > > ebranches;
		$6->swap(ebranches);
		delete $6;

		std::unique_ptr<Expression> afalse;

		for (int i = ebranches.size() - 1; i >= 0; i--) {
			auto& ebranch = ebranches[i];
			afalse.reset(new ConditionalExpression(std::move(ebranch.first), std::move(ebranch.second), std::move(afalse), @6));
		}

		$$ = new ConditionalExpression(std::unique_ptr<Expression>($3), std::unique_ptr<Expression>($5), std::move(afalse), @$);
	}
	| T_IF '(' rterm ')' rterm_scope else_if_branches T_ELSE rterm_scope
	{
		std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression> > > ebranches;
		$6->swap(ebranches);
		delete $6;

		$8->MakeInline();

		std::unique_ptr<Expression> afalse{$8};

		for (int i = ebranches.size() - 1; i >= 0; i--) {
			auto& ebranch = ebranches[i];
			afalse.reset(new ConditionalExpression(std::move(ebranch.first), std::move(ebranch.second), std::move(afalse), @6));
		}

		$$ = new ConditionalExpression(std::unique_ptr<Expression>($3), std::unique_ptr<Expression>($5), std::move(afalse), @$);
	}
	;

rterm_no_side_effect_no_dict: T_STRING
	{
		$$ = MakeLiteralRaw(*$1);
		delete $1;
	}
	| T_NUMBER
	{
		$$ = MakeLiteralRaw($1);
	}
	| T_BOOLEAN
	{
		$$ = MakeLiteralRaw($1);
	}
	| T_NULL
	{
		$$ = MakeLiteralRaw();
	}
	| rterm '.' T_IDENTIFIER %dprec 2
	{
		$$ = new IndexerExpression(std::unique_ptr<Expression>($1), MakeLiteral(*$3), @$);
		delete $3;
	}
	| rterm '[' rterm ']'
	{
		$$ = new IndexerExpression(std::unique_ptr<Expression>($1), std::unique_ptr<Expression>($3), @$);
	}
	| T_IDENTIFIER
	{
		$$ = new VariableExpression(*$1, @1);
		delete $1;
	}
	| '!' rterm
	{
		$$ = new LogicalNegateExpression(std::unique_ptr<Expression>($2), @$);
	}
	| '~' rterm
	{
		$$ = new NegateExpression(std::unique_ptr<Expression>($2), @$);
	}
	| T_PLUS rterm %prec UNARY_PLUS
	{
		$$ = $2;
	}
	| T_MINUS rterm %prec UNARY_MINUS
	{
		$$ = new SubtractExpression(MakeLiteral(0), std::unique_ptr<Expression>($2), @$);
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
	| T_CURRENT_FILENAME
	{
		$$ = MakeLiteralRaw(@$.Path);
	}
	| T_CURRENT_LINE
	{
		$$ = MakeLiteralRaw(@$.FirstLine);
	}
	| identifier T_FOLLOWS
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope %dprec 2
	{
		EndFlowControlBlock(context);

		std::vector<String> args;
		args.push_back(*$1);
		delete $1;

		$$ = new FunctionExpression("<anonymous>", args, {}, std::unique_ptr<Expression>($4), @$);
	}
	| identifier T_FOLLOWS rterm %dprec 1
	{
		ASSERT(!dynamic_cast<DictExpression *>($3));

		std::vector<String> args;
		args.push_back(*$1);
		delete $1;

		$$ = new FunctionExpression("<anonymous>", args, {}, std::unique_ptr<Expression>($3), @$);
	}
	| '(' identifier_items ')' T_FOLLOWS
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope %dprec 2
	{
		EndFlowControlBlock(context);

		$$ = new FunctionExpression("<anonymous>", *$2, {}, std::unique_ptr<Expression>($6), @$);
		delete $2;
	}
	| '(' identifier_items ')' T_FOLLOWS rterm %dprec 1
	{
		ASSERT(!dynamic_cast<DictExpression *>($5));

		$$ = new FunctionExpression("<anonymous>", *$2, {}, std::unique_ptr<Expression>($5), @$);
		delete $2;
	}
	| rterm_array
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
	| T_FUNCTION '(' identifier_items ')' use_specifier
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope
	{
		EndFlowControlBlock(context);

		$$ = new FunctionExpression("<anonymous>", *$3, std::move(*$5), std::unique_ptr<Expression>($7), @$);
		delete $3;
		delete $5;
	}
	| T_NULLARY_LAMBDA_BEGIN
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	statements T_NULLARY_LAMBDA_END
	{
		EndFlowControlBlock(context);

		std::vector<std::unique_ptr<Expression> > dlist;
		decltype(dlist.size()) num = 0;
		for (auto& litem : *$3) {
			if (!litem.second.SideEffect && num != $3->size() - 1)
				yyerror(&litem.second.DebugInfo, NULL, NULL, "Value computed is not used.");
			dlist.push_back(std::move(litem.first));
			num++;
		}
		delete $3;
		std::unique_ptr<DictExpression> aexpr{new DictExpression(std::move(dlist), @$)};
		aexpr->MakeInline();

		$$ = new FunctionExpression("<anonymous>", {}, {}, std::move(aexpr), @$);
	}
	;

rterm_no_side_effect:
	rterm_no_side_effect_no_dict %dprec 1
	| rterm_dict %dprec 2
	{
		std::unique_ptr<Expression> expr{$1};
		BindToScope(expr, ScopeThis);
		$$ = expr.release();
	}
	;

rterm:
	rterm_side_effect %dprec 2
	| rterm_no_side_effect %dprec 1
	;

target_type_specifier: /* empty */
	{
		$$ = new String();
	}
	| T_TO identifier
	{
		$$ = $2;
	}
	;

default_specifier: /* empty */
	{
		$$ = false;
	}
	| T_DEFAULT
	{
		$$ = true;
	}
	;

ignore_specifier: /* empty */
	{
		$$ = false;
	}
	| T_IGNORE_ON_ERROR
	{
		$$ = true;
	}
	;

use_specifier: /* empty */
	{
		$$ = new std::map<String, std::unique_ptr<Expression> >();
	}
	| T_USE '(' use_specifier_items ')'
	{
		$$ = $3;
	}
	;

use_specifier_items: use_specifier_item
	{
		$$ = new std::map<String, std::unique_ptr<Expression> >();
		$$->insert(std::move(*$1));
		delete $1;
	}
	| use_specifier_items ',' use_specifier_item
	{
		$$ = $1;
		$$->insert(std::move(*$3));
		delete $3;
	}
	;

use_specifier_item: identifier
	{
		$$ = new std::pair<String, std::unique_ptr<Expression> >(*$1, std::unique_ptr<Expression>(new VariableExpression(*$1, @1)));
		delete $1;
	}
	| identifier T_SET rterm
	{
		$$ = new std::pair<String, std::unique_ptr<Expression> >(*$1, std::unique_ptr<Expression>($3));
		delete $1;
	}
	;

apply_for_specifier: /* empty */
	| T_FOR '(' identifier T_FOLLOWS identifier T_IN rterm ')'
	{
		context->m_FKVar.top() = *$3;
		delete $3;

		context->m_FVVar.top() = *$5;
		delete $5;

		context->m_FTerm.top() = $7;
	}
	| T_FOR '(' identifier T_IN rterm ')'
	{
		context->m_FKVar.top() = *$3;
		delete $3;

		context->m_FVVar.top() = "";

		context->m_FTerm.top() = $5;
	}
	;

optional_rterm: /* empty */
	{
		$$ = MakeLiteralRaw();
	}
	| rterm
	;

apply:
	{
		context->m_Apply.push(true);
		context->m_SeenAssign.push(false);
		context->m_SeenIgnore.push(false);
		context->m_Assign.push(NULL);
		context->m_Ignore.push(NULL);
		context->m_FKVar.push("");
		context->m_FVVar.push("");
		context->m_FTerm.push(NULL);
	}
	T_APPLY identifier optional_rterm apply_for_specifier target_type_specifier use_specifier ignore_specifier
	{
		BeginFlowControlBlock(context, FlowControlReturn, false);
	}
	rterm_scope_require_side_effect
	{
		EndFlowControlBlock(context);

		context->m_Apply.pop();

		String type = *$3;
		delete $3;
		String target = *$6;
		delete $6;

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

		bool seen_assign = context->m_SeenAssign.top();
		context->m_SeenAssign.pop();

		// assign && !ignore
		if (!seen_assign && !context->m_FTerm.top())
			BOOST_THROW_EXCEPTION(ScriptError("'apply' is missing 'assign'/'for'", DebugInfoRange(@2, @3)));

		std::unique_ptr<Expression> ignore{context->m_Ignore.top()};
		context->m_Ignore.pop();

		std::unique_ptr<Expression> assign;

		if (!seen_assign)
			assign = MakeLiteral(true);
		else
			assign.reset(context->m_Assign.top());

		context->m_Assign.pop();

		std::unique_ptr<Expression> filter;

		if (ignore) {
			std::unique_ptr<Expression>rex{new LogicalNegateExpression(std::move(ignore), DebugInfoRange(@2, @5))};

			filter.reset(new LogicalAndExpression(std::move(assign), std::move(rex), DebugInfoRange(@2, @5)));
		} else
			filter.swap(assign);

		String fkvar = context->m_FKVar.top();
		context->m_FKVar.pop();

		String fvvar = context->m_FVVar.top();
		context->m_FVVar.pop();

		std::unique_ptr<Expression> fterm{context->m_FTerm.top()};
		context->m_FTerm.pop();

		$$ = new ApplyExpression(type, target, std::unique_ptr<Expression>($4), std::move(filter), context->GetPackage(), fkvar, fvvar, std::move(fterm), std::move(*$7), $8, std::unique_ptr<Expression>($10), DebugInfoRange(@2, @8));
		delete $7;
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
