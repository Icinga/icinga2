/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef LANGUAGEPERL_HPP
#define LANGUAGEPERL_HPP

#include "perl/i2-perl.hpp"
#include "base/string.hpp"

extern "C" {
//TODO: Add guards for Windows/Linux - HAVE_EMBEDDED_PERL
# 	include <EXTERN.h>
# 	include <perl.h>
#include <XSUB.h>

// Clear Perl specific macros
#undef DEBUG
#undef ctime
#undef printf
#undef sighandler
#undef localtime
#undef getpwnam
#undef getgrnam
#undef strerror

// This conflicts with std::variant in the Value class
#undef do_open
#undef do_close
#undef bind
#undef seed
#undef push
#undef pop
}

/**
 * This replaces the call to 'perl -MExtUtils::Embed -e xsinit -- -o base/perlxsi.c'
 */
extern "C" {
	void boot_DynaLoader(pTHX_ CV* cv);

	static void xs_init(pTHX)
	{
		dXSUB_SYS;
		PERL_UNUSED_CONTEXT;

		newXS(const_cast<char*>("DynaLoader::boot_DynaLoader"), boot_DynaLoader, const_cast<char*>(__FILE__));
	}
};

namespace icinga
{


class LanguagePerl final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(LanguagePerl);

	LanguagePerl();
	~LanguagePerl();

	static String GetEmbedPath();

	void Initialize();

	void Run(const std::vector<String>& arguments);

	int RunStandalone(const String& fileName, const String& arguments);

	static void Terminate();

private:
	PerlInterpreter* m_Interpreter;

};
}

#endif

