%{
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

Variant *z;
%}

%pure-parser

%locations
%defines
%error-verbose

%parse-param { ConfigContext *context }
%lex-param { void *scanner }

%union {
	char *text;
	int num;
	icinga::Variant *variant;
	icinga::DynamicDictionaryOperator op;
}

%token <text> T_STRING
%token <num> T_NUMBER
%token <text> T_IDENTIFIER
%token T_OPEN_BRACE
%token T_CLOSE_BRACE
%token T_OPEN_BRACKET
%token T_CLOSE_BRACKET
%token <op> T_EQUAL
%token <op> T_PLUS_EQUAL
%token <op> T_MINUS_EQUAL
%token <op> T_MULTIPLY_EQUAL
%token <op> T_DIVIDE_EQUAL
%token T_COMMA
%token T_ABSTRACT
%token T_LOCAL
%token T_OBJECT
%token T_INCLUDE
%token T_INHERITS
%type <variant> value
%type <variant> array
%type <variant> dictionary
%type <op> operator
%left '+' '-'
%left '*' '/'
%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigContext *context, const char *err)
{
	stringstream message;

	message << locp->first_line << ":" << locp->first_column
	    << "-"
	    << locp->last_line << ":" << locp->last_column
	    << ": " << err << endl;

	throw runtime_error(message.str());
}

int yyparse(ConfigContext *context);

void ConfigContext::Compile(void)
{
	yyparse(this);
}

#define scanner (context->GetScanner())

static stack<DynamicDictionary::Ptr> m_Dictionaries;

%}

%%
statements: /* empty */
	| statements statement
	;

statement: object | include
	;

include: T_INCLUDE T_STRING
	;

object: attributes_list object_declaration
	| object_declaration
	;

object_declaration: T_OBJECT T_IDENTIFIER T_STRING inherits_specifier dictionary
	;

attributes_list: attributes_list attribute
	| attribute
	;

attribute: T_ABSTRACT
	| T_LOCAL
	;

inherits_list: T_STRING
	| inherits_list T_COMMA T_STRING
	;

inherits_specifier: /* empty */
	| T_INHERITS inherits_list
	;

dictionary: T_OPEN_BRACE
	{
		DynamicDictionary::Ptr dictionary = make_shared<DynamicDictionary>();
		m_Dictionaries.push(dictionary);
	}
	nvpairs
	T_CLOSE_BRACE
	{
		DynamicDictionary::Ptr dictionary = m_Dictionaries.top();
		$$ = new Variant(dictionary);

		m_Dictionaries.pop();
	}
	;

nvpairs: /* empty */
	| nvpair
	| nvpairs T_COMMA nvpair
	;

nvpair: T_IDENTIFIER operator value
	{
		DynamicDictionary::Ptr dictionary = m_Dictionaries.top();
		dictionary->SetProperty($1, *$3, $2);
		free($1);
		delete $3;
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

value: T_STRING
	{
		$$ = new Variant($1);
	}
	| T_NUMBER
	{
		$$ = new Variant($1);
	}
	| array
	| dictionary
	{
		$$ = $1;
	}
	;

array: T_OPEN_BRACKET
	{
		DynamicDictionary::Ptr dictionary = make_shared<DynamicDictionary>();
		m_Dictionaries.push(dictionary);
	}
	arrayitems
	T_CLOSE_BRACKET
	{
		DynamicDictionary::Ptr dictionary = m_Dictionaries.top();
		$$ = new Variant(dictionary);

		m_Dictionaries.pop();
	}
	;

arrayitems:
	/* empty */
	| value
	{
		DynamicDictionary::Ptr dictionary = m_Dictionaries.top();
		//dictionary->AddUnnamedProperty(*$1);
		delete $1;
	}
	| arrayitems T_COMMA value
	{
		DynamicDictionary::Ptr dictionary = m_Dictionaries.top();
		//dictionary->AddUnnamedProperty(*$3);
		delete $3;
	}
	;
%%
