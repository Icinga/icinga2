// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/sanitizer.hpp"

using namespace icinga;
using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(base_sanitizer)

namespace
{
struct TestCase {
	std::string_view name, input, expected;
};
} // namespace

const auto testcases = {
	TestCase{"empty string", ""sv, ""sv},
	TestCase{"null byte", "\0"sv, R"(\0)"sv},
	TestCase{"null byte with text around it", "hello\0world"sv, R"(hello\0world)"sv},
	TestCase{"string in single quotes", "'single quoted'"sv, R"(\'single quoted\')"sv},
	TestCase{"string in double quotes", R"("double quoted")"sv, R"("double quoted")"sv},
	TestCase{"chars with special escapes", "\0\a\b\f\n\r\t\v'\\"sv, R"(\0\a\b\f\n\r\t\v\'\\)"sv},
	TestCase{"non-ascii unicode is not escaped", "Nürnberg"sv, "Nürnberg"sv},

	TestCase{
		"alphanumeric characters",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"sv,
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"sv,
	},
	TestCase{
		"special characters",
		R"( !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)"sv,
		R"( !"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~)"sv,
	},
	TestCase{
		"control characters",
		"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x7f"sv,
		R"(\0\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x7f)"sv,
	},
	TestCase{
		"all ascii byte values",

		// Input can be generated with Python:
		// print(''.join(f'\\x{i:02x}' for i in range(128)))
		"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
		"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
		"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
		"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
		"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
		"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
		"\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
		"\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"sv,

		// Expected output can be verified with Python (the escape sequences used are compatible with what Python uses):
		// b'\x00[...]\xff' == bytes(range(128))
		R"(\0\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0e\x0f)"
		R"(\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f)"
		R"( !"#$%&\'()*+,-./0123456789:;<=>?)"		// 0x20..0x3f, `'` is escaped here
		R"(@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_)"		// 0x40..0x5f, `\` is escaped here
		R"(`abcdefghijklmnopqrstuvwxyz{|}~\x7f)"sv, // 0x60..0x7f
	},
	TestCase{
		"all non-ascii byte values",

		// Input can be generated with Python:
		// print(''.join(f'\\x{i:02x}' for i in range(128, 256)))
		"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
		"\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
		"\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
		"\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
		"\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
		"\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
		"\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
		"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"sv,

		// Same as the input
		"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
		"\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
		"\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
		"\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
		"\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
		"\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
		"\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
		"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"sv,
	},
};

BOOST_AUTO_TEST_CASE(sanitizer_stream_without_quotes)
{
	for (const auto& t : testcases) {
		BOOST_TEST_CONTEXT("Test Case: " << t.name)
		{
			std::stringstream stream;
			Sanitizer::WriteToStreamWithoutQuotes(stream, t.input);
			BOOST_TEST_MESSAGE("input length " << t.input.size());
			BOOST_TEST_MESSAGE("expected output length " << t.expected.size());
			BOOST_CHECK_EQUAL(stream.str(), t.expected);
		}
	}
}

BOOST_AUTO_TEST_CASE(sanitizer_stream_with_quotes)
{
	for (const auto& t : testcases) {
		BOOST_TEST_CONTEXT("Test Case: " << t.name)
		{
			std::stringstream stream;
			Sanitizer::WriteToStreamWithQuotes(stream, t.input);
			BOOST_TEST_MESSAGE("input length " << t.input.size());
			BOOST_TEST_MESSAGE("expected output length " << t.expected.size());
			BOOST_CHECK_EQUAL(stream.str(), "'" + std::string(t.expected) + "'");
		}
	}
}

BOOST_AUTO_TEST_CASE(sanitizer_string_without_quotes)
{
	for (const auto& t : testcases) {
		BOOST_TEST_CONTEXT("Test Case: " << t.name)
		{
			BOOST_CHECK_EQUAL(Sanitizer::UnquotedString(t.input).GetData(), t.expected);
		}
	}
}

BOOST_AUTO_TEST_CASE(sanitizer_string_with_quotes)
{
	for (const auto& t : testcases) {
		BOOST_TEST_CONTEXT("Test Case: " << t.name)
		{
			BOOST_CHECK_EQUAL(Sanitizer::QuotedString(t.input).GetData(), "'" + std::string(t.expected) + "'");
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
