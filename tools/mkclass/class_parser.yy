%{
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
	FieldType *type;
	Field *field;
	std::vector<Field> *fields;
	Klass *klass;
	FieldAccessor *fieldaccessor;
	std::vector<FieldAccessor> *fieldaccessors;
	Rule *rule;
	std::vector<Rule> *rules;
	Validator *validator;
}

%token T_INCLUDE "#include (T_INCLUDE)"
%token T_IMPL_INCLUDE "#impl_include (T_IMPL_INCLUDE)"
%token T_CLASS "class (T_CLASS)"
%token T_CODE "code (T_CODE)"
%token T_LOAD_AFTER "load_after (T_LOAD_AFTER)"
%token T_LIBRARY "library (T_LIBRARY)"
%token T_NAMESPACE "namespace (T_NAMESPACE)"
%token T_VALIDATOR "validator (T_VALIDATOR)"
%token T_REQUIRED "required (T_REQUIRED)"
%token T_NAVIGATION "navigation (T_NAVIGATION)"
%token T_NAME "name (T_NAME)"
%token T_ARRAY "array (T_ARRAY)"
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
%type <text> impl_include
%type <text> angle_impl_include
%type <text> code
%type <num> T_FIELD_ATTRIBUTE
%type <field> field_attribute
%type <field> field_attributes
%type <field> field_attribute_list
%type <num> T_FIELD_ACCESSOR_TYPE
%type <num> T_CLASS_ATTRIBUTE
%type <num> class_attribute_list
%type <type> field_type
%type <field> class_field
%type <fields> class_fields
%type <klass> class
%type <fieldaccessors> field_accessor_list
%type <fieldaccessors> field_accessors
%type <fieldaccessor> field_accessor
%type <rule> validator_rule
%type <rules> validator_rules
%type <validator> validator

%{

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ClassCompiler *, const char *err)
{
	std::cerr << "in " << locp->path << " at " << locp->first_line << ":" << locp->first_column << "-"
		<< locp->last_line << ":" << locp->last_column << ": " << err << std::endl;
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

	HandleMissingValidators();
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
	| impl_include
	{
		context->HandleImplInclude($1, yylloc);
		std::free($1);
	}
	| angle_impl_include
	{
		context->HandleAngleImplInclude($1, yylloc);
		std::free($1);
	}
	| class
	{
		context->HandleClass(*$1, yylloc);
		delete $1;
	}
	| validator
	{
		context->HandleValidator(*$1, yylloc);
		delete $1;
	}
	| namespace
	| code
	{
		context->HandleCode($1, yylloc);
		std::free($1);
	}
	| library
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

impl_include: T_IMPL_INCLUDE T_STRING
	{
		$$ = $2;
	}
	;

angle_impl_include: T_IMPL_INCLUDE T_ANGLE_STRING
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

library: T_LIBRARY T_IDENTIFIER ';'
	{
		context->HandleLibrary($2, yylloc);
		free($2);
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

		for (const Field& field : *$7) {
			if (field.Attributes & FALoadDependency) {
				$$->LoadDependencies.push_back(field.Name);
			} else
				$$->Fields.push_back(field);
		}

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

field_type: identifier
	{
		$$ = new FieldType();
		$$->IsName = false;
		$$->TypeName = $1;
		free($1);
	}
	| T_NAME '(' identifier ')'
	{
		$$ = new FieldType();
		$$->IsName = true;
		$$->TypeName = $3;
		$$->ArrayRank = 0;
		free($3);
	}
	| T_ARRAY '(' field_type ')'
	{
		$$ = $3;
		$$->ArrayRank++;
	}
	;

class_field: field_attribute_list field_type identifier alternative_name_specifier field_accessor_list ';'
	{
		Field *field = $1;

		if ((field->Attributes & (FAConfig | FAState)) == 0)
			field->Attributes |= FAEphemeral;

		field->Type = *$2;
		delete $2;

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
				case FTTrack:
					field->TrackAccessor = it->Accessor;
					break;
				case FTNavigate:
					field->NavigateAccessor = it->Accessor;
					field->PureNavigateAccessor = it->Pure;
					break;
			}
		}

		delete $5;

		$$ = field;
	}
	| T_LOAD_AFTER identifier ';'
	{
		auto *field = new Field();
		field->Attributes = FALoadDependency;
		field->Name = $2;
		std::free($2);
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
		$$ = new Field();
	}
	| '[' field_attributes ']'
	{
		$$ = $2;
	}
	;

field_attribute: T_FIELD_ATTRIBUTE
	{
		$$ = new Field();
		$$->Attributes = $1;
	}
	| T_REQUIRED
	{
		$$ = new Field();
		$$->Attributes = FARequired;
	}
	| T_NAVIGATION '(' identifier ')'
	{
		$$ = new Field();
		$$->Attributes = FANavigation;
		$$->NavigationName = $3;
		std::free($3);
	}
	| T_NAVIGATION
	{
		$$ = new Field();
		$$->Attributes = FANavigation;
	}
	;

field_attributes: /* empty */
	{
		$$ = new Field();
	}
	| field_attributes ',' field_attribute
	{
		$$ = $1;
		$$->Attributes |= $3->Attributes;
		if (!$3->NavigationName.empty())
			$$->NavigationName = $3->NavigationName;
		delete $3;
	}
	| field_attribute
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

validator_rules: /* empty */
	{
		$$ = new std::vector<Rule>();
	}
	| validator_rules validator_rule
	{
		$$->push_back(*$2);
		delete $2;
	}
	;

validator_rule: T_NAME '(' T_IDENTIFIER ')' identifier ';'
	{
		$$ = new Rule();
		$$->Attributes = 0;
		$$->IsName = true;
		$$->Type = $3;
		std::free($3);
		$$->Pattern = $5;
		std::free($5);
	}
	| T_IDENTIFIER identifier ';'
	{
		$$ = new Rule();
		$$->Attributes = 0;
		$$->IsName = false;
		$$->Type = $1;
		std::free($1);
		$$->Pattern = $2;
		std::free($2);
	}
	| T_NAME '(' T_IDENTIFIER ')' identifier '{' validator_rules '}' ';'
	{
		$$ = new Rule();
		$$->Attributes = 0;
		$$->IsName = true;
		$$->Type = $3;
		std::free($3);
		$$->Pattern = $5;
		std::free($5);
		$$->Rules = *$7;
		delete $7;
	}
	| T_IDENTIFIER identifier '{' validator_rules '}' ';'
	{
		$$ = new Rule();
		$$->Attributes = 0;
		$$->IsName = false;
		$$->Type = $1;
		std::free($1);
		$$->Pattern = $2;
		std::free($2);
		$$->Rules = *$4;
		delete $4;
	}
	| T_REQUIRED identifier ';'
	{
		$$ = new Rule();
		$$->Attributes = RARequired;
		$$->IsName = false;
		$$->Type = "";
		$$->Pattern = $2;
		std::free($2);
	}
	;

validator: T_VALIDATOR T_IDENTIFIER '{' validator_rules '}' ';'
	{
		$$ = new Validator();

		$$->Name = $2;
		std::free($2);

		$$->Rules = *$4;
		delete $4;
	}
	;

identifier: T_IDENTIFIER
	| T_STRING
	{
		$$ = $1;
	}
	;
