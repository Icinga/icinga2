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

#include "base/stacktrace.hpp"
#include "base/utility.hpp"
#include "base/initialize.hpp"

#ifdef HAVE_BACKTRACE_SYMBOLS
#	include <execinfo.h>
#endif /* HAVE_BACKTRACE_SYMBOLS */

using namespace icinga;

#ifdef _MSC_VER
#	pragma optimize("", off)
#endif /* _MSC_VER */

StackTrace::StackTrace()
{
#ifdef HAVE_BACKTRACE_SYMBOLS
	m_Count = backtrace(m_Frames, sizeof(m_Frames) / sizeof(m_Frames[0]));
#else /* HAVE_BACKTRACE_SYMBOLS */
#	ifdef _WIN32
	m_Count = CaptureStackBackTrace(0, sizeof(m_Frames) / sizeof(m_Frames), m_Frames, nullptr);
#	else /* _WIN32 */
	m_Count = 0;
#	endif /* _WIN32 */
#endif /* HAVE_BACKTRACE_SYMBOLS */
}

#ifdef _MSC_VER
#	pragma optimize("", on)
#endif /* _MSC_VER */

#ifdef _WIN32
StackTrace::StackTrace(PEXCEPTION_POINTERS exi)
{
	STACKFRAME64 frame;
	int architecture;

#ifdef _WIN64
	architecture = IMAGE_FILE_MACHINE_AMD64;

	frame.AddrPC.Offset = exi->ContextRecord->Rip;
	frame.AddrFrame.Offset = exi->ContextRecord->Rbp;
	frame.AddrStack.Offset = exi->ContextRecord->Rsp;
#else /* _WIN64 */
	architecture = IMAGE_FILE_MACHINE_I386;

	frame.AddrPC.Offset = exi->ContextRecord->Eip;
	frame.AddrFrame.Offset = exi->ContextRecord->Ebp;
	frame.AddrStack.Offset = exi->ContextRecord->Esp;
#endif  /* _WIN64 */

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;

	m_Count = 0;

	while (StackWalk64(architecture, GetCurrentProcess(), GetCurrentThread(),
		&frame, exi->ContextRecord, nullptr, &SymFunctionTableAccess64,
		&SymGetModuleBase64, nullptr) && m_Count < sizeof(m_Frames) / sizeof(m_Frames[0])) {
		m_Frames[m_Count] = reinterpret_cast<void *>(frame.AddrPC.Offset);
		m_Count++;
	}
}
#endif /* _WIN32 */

#ifdef _WIN32
INITIALIZE_ONCE([]() {
	(void) SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
	(void) SymInitialize(GetCurrentProcess(), nullptr, TRUE);
});
#endif /* _WIN32 */

/**
 * Prints a stacktrace to the specified stream.
 *
 * @param fp The stream.
 * @param ignoreFrames The number of stackframes to ignore (in addition to
 *        the one this function is executing in).
 * @returns true if the stacktrace was printed, false otherwise.
 */
void StackTrace::Print(std::ostream& fp, int ignoreFrames) const
{
	fp << std::endl;

#ifndef _WIN32
#	ifdef HAVE_BACKTRACE_SYMBOLS
	char **messages = backtrace_symbols(m_Frames, m_Count);

	for (int i = ignoreFrames + 1; i < m_Count && messages; ++i) {
		String message = messages[i];

		char *sym_begin = strchr(messages[i], '(');

		if (sym_begin) {
			char *sym_end = strchr(sym_begin, '+');

			if (sym_end) {
				String sym = String(sym_begin + 1, sym_end);
				String sym_demangled = Utility::DemangleSymbolName(sym);

				if (sym_demangled.IsEmpty())
					sym_demangled = "<unknown function>";

				String path = String(messages[i], sym_begin);

				size_t slashp = path.RFind("/");

				if (slashp != String::NPos)
					path = path.SubStr(slashp + 1);

				message = path + ": " + sym_demangled + " (" + String(sym_end);
			}
		}

		fp << "\t(" << i - ignoreFrames - 1 << ") " << message << std::endl;
	}

	std::free(messages);

	fp << std::endl;
#	else /* HAVE_BACKTRACE_SYMBOLS */
	fp << "(not available)" << std::endl;
#	endif /* HAVE_BACKTRACE_SYMBOLS */
#else /* _WIN32 */
	for (int i = ignoreFrames + 1; i < m_Count; i++) {
		fp << "\t(" << i - ignoreFrames - 1 << "): " << Utility::GetSymbolName(m_Frames[i]) << std::endl;
	}
#endif /* _WIN32 */
}

std::ostream& icinga::operator<<(std::ostream& stream, const StackTrace& trace)
{
	trace.Print(stream, 1);

	return stream;
}
