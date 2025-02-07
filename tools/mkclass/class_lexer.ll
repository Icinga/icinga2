%{
/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "classcompiler.hpp"

using namespace icinga;

#define YYLTYPE icinga::ClassDebugInfo

#include "class_parser.hh"

#define YY_EXTRA_TYPE ClassCompiler *
#define YY_USER_ACTION 					\
do {							\
	yylloc->path = yyextra->GetPath();		\
	yylloc->first_line = yylineno;			\
	yylloc->first_column = yycolumn;		\
	yylloc->last_line = yylineno;			\
	yylloc->last_column = yycolumn + yyleng - 1;	\
	yycolumn += yyleng;				\
} while (0);

#define YY_INPUT(buf, result, max_size)			\
do {							\
	result = yyextra->ReadInput(buf, max_size);	\
} while (0)

struct lex_buf {
	char *buf;
	size_t size;
};

static void lb_init(lex_buf *lb)
{
	lb->buf = NULL;
	lb->size = 0;
}

/*static void lb_cleanup(lex_buf *lb)
{
	free(lb->buf);
}*/

static void lb_append_char(lex_buf *lb, char new_char)
{
	const size_t block_size = 64;

	size_t old_blocks = (lb->size + (block_size - 1)) / block_size;
	size_t new_blocks = ((lb->size + 1) + (block_size - 1)) / block_size;

	if (old_blocks != new_blocks) {
		char *new_buf = (char *)realloc(lb->buf, new_blocks * block_size);

		if (new_buf == NULL && new_blocks > 0)
			throw std::bad_alloc();

		lb->buf = new_buf;
	}

	lb->size++;
	lb->buf[lb->size - 1] = new_char;
}

static char *lb_steal(lex_buf *lb)
{
	lb_append_char(lb, '\0');

	char *buf = lb->buf;
	lb->buf = NULL;
	lb->size = 0;
	return buf;
}
%}

%option reentrant noyywrap yylineno
%option bison-bridge bison-locations
%option never-interactive nounistd
%option noinput nounput

%x HEREDOC
%x C_COMMENT

%%
	lex_buf string_buf;

\{\{\{				{ lb_init(&string_buf); BEGIN(HEREDOC); }

<HEREDOC>\}\}\}			{
	BEGIN(INITIAL);

	lb_append_char(&string_buf, '\0');

	yylval->text = lb_steal(&string_buf);

	return T_STRING;
				}

<HEREDOC>(.|\n)			{ lb_append_char(&string_buf, yytext[0]); }

"/*"				{ BEGIN(C_COMMENT); }

<C_COMMENT>{
"*/"				{ BEGIN(INITIAL); }
[^*]				/* ignore comment */
"*"				/* ignore star */
}

<C_COMMENT><<EOF>>              {
	fprintf(stderr, "End-of-file while in comment.\n");
	yyterminate();
				}
\/\/[^\n]*                      /* ignore C++-style comments */
[ \t\r\n]			/* ignore whitespace */

#include			{ return T_INCLUDE; }
#impl_include			{ return T_IMPL_INCLUDE; }
class				{ return T_CLASS; }
namespace			{ return T_NAMESPACE; }
code				{ return T_CODE; }
load_after			{ return T_LOAD_AFTER; }
activation_priority	{ return T_ACTIVATION_PRIORITY; }
library				{ return T_LIBRARY; }
abstract			{ yylval->num = TAAbstract; return T_CLASS_ATTRIBUTE; }
vararg_constructor		{ yylval->num = TAVarArgConstructor; return T_CLASS_ATTRIBUTE; }
config				{ yylval->num = FAConfig; return T_FIELD_ATTRIBUTE; }
state				{ yylval->num = FAState; return T_FIELD_ATTRIBUTE; }
enum				{ yylval->num = FAEnum; return T_FIELD_ATTRIBUTE; }
get_protected			{ yylval->num = FAGetProtected; return T_FIELD_ATTRIBUTE; }
set_protected			{ yylval->num = FASetProtected; return T_FIELD_ATTRIBUTE; }
protected			{ yylval->num = FAGetProtected | FASetProtected; return T_FIELD_ATTRIBUTE; }
no_storage			{ yylval->num = FANoStorage; return T_FIELD_ATTRIBUTE; }
no_user_modify			{ yylval->num = FANoUserModify; return T_FIELD_ATTRIBUTE; }
no_user_view			{ yylval->num = FANoUserView; return T_FIELD_ATTRIBUTE; }
deprecated			{ yylval->num = FADeprecated; return T_FIELD_ATTRIBUTE; }
get_virtual			{ yylval->num = FAGetVirtual; return T_FIELD_ATTRIBUTE; }
set_virtual			{ yylval->num = FASetVirtual; return T_FIELD_ATTRIBUTE; }
signal_with_old_value			{ yylval->num = FASignalWithOldValue; return T_FIELD_ATTRIBUTE; }
virtual				{ yylval->num = FAGetVirtual | FASetVirtual; return T_FIELD_ATTRIBUTE; }
notify_args			{ yylval->num = FANotifyArgs; return T_FIELD_ATTRIBUTE; }
navigation			{ return T_NAVIGATION; }
validator			{ return T_VALIDATOR; }
required			{ return T_REQUIRED; }
name				{ return T_NAME; }
array				{ return T_ARRAY; }
default				{ yylval->num = FTDefault; return T_FIELD_ACCESSOR_TYPE; }
get				{ yylval->num = FTGet; return T_FIELD_ACCESSOR_TYPE; }
set				{ yylval->num = FTSet; return T_FIELD_ACCESSOR_TYPE; }
track				{ yylval->num = FTTrack; return T_FIELD_ACCESSOR_TYPE; }
navigate			{ yylval->num = FTNavigate; return T_FIELD_ACCESSOR_TYPE; }
\"[^\"]+\"			{ yylval->text = strdup(yytext + 1); yylval->text[strlen(yylval->text) - 1] = '\0'; return T_STRING; }
\<[^ \>]*\>			{ yylval->text = strdup(yytext + 1); yylval->text[strlen(yylval->text) - 1] = '\0'; return T_ANGLE_STRING; }
[a-zA-Z_][:a-zA-Z0-9\-_]*	{ yylval->text = strdup(yytext); return T_IDENTIFIER; }
-?[0-9]+(\.[0-9]+)?		{ yylval->num = strtod(yytext, NULL); return T_NUMBER; }

.				return yytext[0];

%%

void ClassCompiler::InitializeScanner(void)
{
	yylex_init(&m_Scanner);
	yyset_extra(this, m_Scanner);
}

void ClassCompiler::DestroyScanner(void)
{
	yylex_destroy(m_Scanner);
}
