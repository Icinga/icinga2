// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/stream.hpp>
#include <ostream>
#include <sstream>

namespace icinga
{

/**
 * Helper class for sanitizing input values for inclusion in log messages and other strings.
 *
 * @ingroup base
 */
class Sanitizer
{
public:
	template<typename T>
	static void WriteToStreamWithoutQuotes(std::ostream& stream, T&& value)
	{
		namespace io = boost::iostreams;
		io::stream{io::compose(QuotedStringFilter{}, stream)} << std::forward<T>(value);
	}

	template<typename T>
	static void WriteToStreamWithQuotes(std::ostream& stream, T&& value)
	{
		stream << '\'';
		WriteToStreamWithoutQuotes(stream, std::forward<T>(value));
		stream << '\'';
	}

	template<typename T>
	static String UnquotedString(T&& value)
	{
		std::ostringstream stream;
		WriteToStreamWithoutQuotes(stream, std::forward<T>(value));
		return stream.str();
	}

	template<typename T>
	static String QuotedString(T&& value)
	{
		std::ostringstream stream;
		WriteToStreamWithQuotes(stream, std::forward<T>(value));
		return stream.str();
	}

	template<typename T>
	static void WriteLogMessage(std::ostream& stream, T&& value)
	{
		namespace io = boost::iostreams;
		io::stream{io::compose(LogMessageFilter{}, stream)} << std::forward<T>(value);
	}

	template<typename T>
	static void WriteContextFrame(std::ostream& stream, T&& value)
	{
		namespace io = boost::iostreams;
		io::stream{io::compose(ContextLineFilter{}, stream)} << std::forward<T>(value);
	}

	static bool IsAsciiControl(unsigned char c) { return c < 0x20 || c == 0x7f; }

private:
	template<typename Replacements>
	class Filter : public boost::iostreams::output_filter
	{
	public:
		template<typename Sink>
		bool put(Sink& dest, char c)
		{
			using namespace std::string_view_literals;

			auto write = [&dest](std::string_view v) {
				return boost::iostreams::write(dest, v.data(), v.size()) == static_cast<std::streamsize>(v.size());
			};

			switch (c) {
				case '\t':
					return write(Replacements::tab);
				case '\n':
					return write(Replacements::new_line);
				case '\'':
					return write(Replacements::single_quote);

				case '\0':
					return write("\\0"sv);
				case '\a':
					return write("\\a"sv);
				case '\b':
					return write("\\b"sv);
				case '\f':
					return write("\\f"sv);
				case '\r':
					return write("\\r"sv);
				case '\v':
					return write("\\v"sv);
				case '\\':
					return write("\\\\"sv);

				default:
					// This if covers all ASCII control characters that haven't already been handled above.
					if (const unsigned char uc = c; IsAsciiControl(c)) {
						constexpr auto hexDigits = "0123456789abcdef";
						const std::array encoded = {
							'\\',
							'x',
							hexDigits[uc >> 4],
							hexDigits[uc & 0xf],
						};
						return write(std::string_view(encoded.data(), encoded.size()));
					}

					// The remaining ASCII printable characters and non-ASCII characters (bytes with their highest bit
					// set), are passed through as-is. In UTF-8, the latter are part of multi-byte encodings of higher
					// unicode codepoints (or it could be invalid UTF-8). Printing them without escaping allows keeping
					// characters from languages that can't be (fully) represented in ASCII intact. It might be possible
					// to do some annoying stuff in the log files with unicode, but the integrity of the log files is
					// guaranteed as you can at least always figure out what's going on by looking at the raw bytes.
					return boost::iostreams::put(dest, c);
			}
		}
	};

	// Escaping rules for full log messages. Quoting of individual values was already done and they are passed through
	// as-is. Line breaks are also kept to allow for multi-line messages, but add an extra indentation using a TAB
	// to clearly mark the line as a continuation and provide a second line of defense against log injections.
	struct LogMessageReplacements {
		static constexpr std::string_view tab = "\t";
		static constexpr std::string_view new_line = "\n\t";
		static constexpr std::string_view single_quote = "'";
	};
	using LogMessageFilter = Filter<LogMessageReplacements>;

	// Escaping rules for quoted string values so that they can be safely used in log messages for example. As Icinga 2
	// log messages typically use single quoted strings, it follows that convention and is otherwise similar to how JSON
	// encodes strings.
	struct QuotedStringReplacements {
		static constexpr std::string_view tab = "\\t";
		static constexpr std::string_view new_line = "\\n";
		static constexpr std::string_view single_quote = "\\'";
	};
	using QuotedStringFilter = Filter<QuotedStringReplacements>;

	// Special escaping rules used for adding extra context information to log messages. Ensures that control characters
	// are escaped, including '\n' to guarantee that the context is only a single line. Apart from that, the value is
	// left mostly as-is.
	struct ContextLineReplacements {
		static constexpr std::string_view tab = "\t";
		static constexpr std::string_view new_line = "\\n";
		static constexpr std::string_view single_quote = "'";
	};
	using ContextLineFilter = Filter<ContextLineReplacements>;
};

} // namespace icinga
