%pure-parser

%locations
%defines
%error-verbose

%parse-param { ConfigContext *context }
%lex-param { void *scanner }

%union {
	char *text;
	int num;
}

%token <text> T_STRING
%token <num> T_NUMBER
%token T_IDENTIFIER
%token T_OPEN_BRACE
%token T_CLOSE_BRACE
%token T_OPEN_BRACKET
%token T_CLOSE_BRACKET
%token T_EQUAL
%token T_COMMA
%token T_ABSTRACT
%token T_LOCAL
%token T_OBJECT
%token T_INCLUDE
%token T_INHERITS

%{
#include "i2-configfile.h"

using namespace icinga;

int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner);

void yyerror(YYLTYPE *locp, ConfigContext *context, const char *err)
{
	std::cout << locp->first_line << ":" << locp->first_column
	    << "-"
	    << locp->last_line << ":" << locp->last_column
	    << ": " << err << std::endl;
}

#define scanner context->Scanner

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

dictionary: T_OPEN_BRACE nvpairs T_CLOSE_BRACE
	;

nvpairs: /* empty */
	| nvpair
	| nvpairs T_COMMA nvpair
	;

nvpair: T_IDENTIFIER T_EQUAL value
	;

value: T_STRING | T_NUMBER | array | dictionary
	;

array: T_OPEN_BRACKET arrayitems T_CLOSE_BRACKET
	;

arrayitems:
	/* empty */
	| value
	| arrayitems T_COMMA value
	;
%%
