%{
#include "i2-dyn.h"
#include "config_parser.h"

using namespace icinga;

#define YY_EXTRA_TYPE ConfigContext *
#define YY_USER_ACTION yylloc->first_line = yylineno;

#define YY_INPUT(buf, result, max_size)			\
do {							\
	result = yyextra->ReadInput(buf, max_size);	\
} while (0)
%}

%option reentrant noyywrap yylineno
%option bison-bridge bison-locations
%option never-interactive nounistd

%x IN_C_COMMENT

%%
abstract		return T_ABSTRACT;
local			return T_LOCAL;
object			return T_OBJECT;
include			return T_INCLUDE;
inherits		return T_INHERITS;
[a-zA-Z][a-zA-Z0-9]*	return T_IDENTIFIER;
\"[^\"]+\"		{ yytext[yyleng-1] = '\0'; yylval->text = strdup(yytext + 1); return T_STRING; }
[0-9]+			{ yylval->num = atoi(yytext); return T_NUMBER; }
\{			return T_OPEN_BRACE;
\}			return T_CLOSE_BRACE;
\[			return T_OPEN_BRACKET;
\]			return T_CLOSE_BRACKET;
,			return T_COMMA;
=			return T_EQUAL;

<INITIAL>{
"/*"			BEGIN(IN_C_COMMENT);
}

<IN_C_COMMENT>{
"*/"			BEGIN(INITIAL);
[^*]+			/* ignore comment */
"*"			/* ignore star */
}

\/\/[^\n]+		/* ignore C++-style comments */
#[^\n]+			/* ignore shell-style comments */
[ \t\n]+		/* ignore whitespace */
%%


void ConfigContext::InitializeScanner(void)
{
	yylex_init(&m_Scanner);
	yyset_extra(this, m_Scanner);
}

void ConfigContext::DestroyScanner(void)
{
	yylex_destroy(m_Scanner);
}

