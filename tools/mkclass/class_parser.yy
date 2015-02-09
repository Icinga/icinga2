%{
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

#include "classcompiler.hpp"
#include <iostream>
#include <vector>
#include <cstring>

using std::malloc;
using std::free;
using std::exit;

using namespace icinga;

#define YYLTYPE icinga::ClassDebugInfo

%}

%pure-parser

%locations
%defines
%error-verbose

%parse-param { ClassCompiler *context }
%lex-param { void *scanner }

%union {
	char *text;
	int num;
	Field *field;
	std::vector<Field> *fields;
	Klass *klass;
	FieldAccessor *fieldaccessor;
	std::vector<FieldAccessor> *fieldaccessors;
}

%token T_INCLUDE "include (T_INCLUDE)"
%token T_CLASS "class (T_CLASS)"
%token T_CODE "code (T_CODE)"
%token T_NAMESPACE "namespace (T_NAMESPACE)"
%token T_STRING "string (T_STRING)"
%token T_ANGLE_STRING "angle_string (T_ANGLE_STRING)"
%token T_FIELD_ATTRIBUTE "field_attribute (T_FIELD_ATTRIBUTE)"
%token T_CLASS_ATTRIBUTE "class_attribute (T_CLASS_ATTRIBUTE)"
%token T_IDENTIFIER "identifier (T_IDENTIFIER)"
%token T_GET "get (T_GET)"
%token T_SET "set (T_SET)"
%token T_DEFAULT "default (T_DEFAULT)"
%token T_FIELD_ACCESSOR_TYPE "field_accessor_type (T_FIELD_ACCESSOR_TYPE)"
%type <text> T_IDENTIFIER
%type <text> T_STRING
%type <text> T_ANGLE_STRING
%type <text> identifier
%type <text> alternative_name_specifier
%type <text> inherits_specifier
%type <text> type_base_specifier
%type <text> include
%type <text> angle_include
%type <text> code
%type <num> T_FIELD_ATTRIBUTE
%type <num> field_attributes
%type <num> field_attribute_list
%type <num> T_FIELD_ACCESSOR_TYPE
%type <num> T_CLASS_ATTRIBUTE
%type <num> class_attribute_list
%type <field> class_field
%type <fields> class_fields
%type <klass> class
%type <fieldaccessors> field_accessor_list
%type <fieldaccessors> field_accessors
%type <fieldaccessor> field_accessor

%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ClassCompiler *, const char *err)
{
	std::cerr << "in " << locp->path << " at " << locp->first_line << ":" << locp->first_column << "-" << locp->last_line << ":" << locp->last_column << ": "
			  << err
			  << std::endl;
	std::exit(1);
}

int yyparse(ClassCompiler *context);

void ClassCompiler::Compile(void)
{
	try {
		yyparse(this);
	} catch (const std::exception& ex) {
		std::cerr << "Exception: " << ex.what();
	}
}

#define scanner (context->GetScanner())

%}

%%

statements: /* empty */
	| statements statement
	;

statement: include
	{
		context->HandleInclude($1, yylloc);
		std::free($1);
	}
	| angle_include
	{
		context->HandleAngleInclude($1, yylloc);
		std::free($1);
	}
	| class
	{
		context->HandleClass(*$1, yylloc);
		delete $1;
	}
	| namespace
	| code
	{
		context->HandleCode($1, yylloc);
		std::free($1);
	}
	;

include: T_INCLUDE T_STRING
	{
		$$ = $2;
	}
	;

angle_include: T_INCLUDE T_ANGLE_STRING
	{
		$$ = $2;
	}
	;

namespace: T_NAMESPACE identifier '{'
	{
		context->HandleNamespaceBegin($2, yylloc);
		std::free($2);
	}
	statements '}'
	{
		context->HandleNamespaceEnd(yylloc);
	}
	;

code: T_CODE T_STRING
	{
		$$ = $2;
	}
	;

class: class_attribute_list T_CLASS T_IDENTIFIER inherits_specifier type_base_specifier '{' class_fields '}' ';'
	{
		$$ = new Klass();

		$$->Name = $3;
		std::free($3);

		if ($4) {
			$$->Parent = $4;
			std::free($4);
		}

		if ($5) {
			$$->TypeBase = $5;
			std::free($5);
		}

		$$->Attributes = $1;

		$$->Fields = *$7;
		delete $7;

		ClassCompiler::OptimizeStructLayout($$->Fields);
	}
	;

class_attribute_list: /* empty */
	{
		$$ = 0;
	}
	| T_CLASS_ATTRIBUTE
	{
		$$ = $1;
	}
	| class_attribute_list T_CLASS_ATTRIBUTE
	{
		$$ = $1 | $2;
	}

inherits_specifier: /* empty */
	{
		$$ = NULL;
	}
	| ':' identifier
	{
		$$ = $2;
	}
	;

type_base_specifier: /* empty */
	{
		$$ = NULL;
	}
	| '<' identifier
	{
		$$ = $2;
	}
	;

class_fields: /* empty */
	{
		$$ = new std::vector<Field>();
	}
	| class_fields class_field
	{
		$$->push_back(*$2);
		delete $2;
	}
	;

class_field: field_attribute_list identifier identifier alternative_name_specifier field_accessor_list ';'
	{
		Field *field = new Field();

		field->Attributes = $1;

		field->Type = $2;
		std::free($2);

		field->Name = $3;
		std::free($3);

		if ($4) {
			field->AlternativeName = $4;
			std::free($4);
		}

		std::vector<FieldAccessor>::const_iterator it;
		for (it = $5->begin(); it != $5->end(); it++) {
			switch (it->Type) {
				case FTGet:
					field->GetAccessor = it->Accessor;
					field->PureGetAccessor = it->Pure;
					break;
				case FTSet:
					field->SetAccessor = it->Accessor;
					field->PureSetAccessor = it->Pure;
					break;
				case FTDefault:
					field->DefaultAccessor = it->Accessor;
					break;
				}
		}

		delete $5;

		$$ = field;
	}
	;

alternative_name_specifier: /* empty */
	{
		$$ = NULL;
	}
	| '(' identifier ')'
	{
		$$ = $2;
	}
	;

field_attribute_list: /* empty */
	{
		$$ = 0;
	}
	| '[' field_attributes ']'
	{
		$$ = $2;
	}
	;

field_attributes: /* empty */
	{
		$$ = 0;
	}
	| field_attributes ',' T_FIELD_ATTRIBUTE
	{
		$$ = $1 | $3;
	}
	| T_FIELD_ATTRIBUTE
	{
		$$ = $1;
	}
	;

field_accessor_list: /* empty */
	{
		$$ = new std::vector<FieldAccessor>();
	}
	| '{' field_accessors '}'
	{
		$$ = $2;
	}
	;

field_accessors: /* empty */
	{
		$$ = new std::vector<FieldAccessor>();
	}
	| field_accessors field_accessor
	{
		$$ = $1;
		$$->push_back(*$2);
		delete $2;
	}
	;

field_accessor: T_FIELD_ACCESSOR_TYPE T_STRING
	{
		$$ = new FieldAccessor(static_cast<FieldAccessorType>($1), $2, false);
		std::free($2);
	}
	| T_FIELD_ACCESSOR_TYPE ';'
	{
		$$ = new FieldAccessor(static_cast<FieldAccessorType>($1), "", true);
	}
	;

identifier: T_IDENTIFIER
	| T_STRING
	{
		$$ = $1;
	}
	;
