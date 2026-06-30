// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/object-packer.hpp"
#include "base/value.hpp"
#include "base/string.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <BoostTestTargetConfig.h>
#include <climits>
#include <initializer_list>
#include <iomanip>
#include <sstream>

using namespace icinga;

#if CHAR_MIN != 0
union CharU2SConverter
{
	CharU2SConverter()
	{
		s = 0;
	}

	unsigned char u;
	signed char s;
};
#endif

/**
 * Avoid implementation-defined overflows during unsigned to signed casts
 */
static inline char UIntToByte(unsigned i)
{
#if CHAR_MIN == 0
	return i;
#else
	CharU2SConverter converter;

	converter.u = i;
	return converter.s;
#endif
}

#if CHAR_MIN != 0
union CharS2UConverter
{
	CharS2UConverter()
	{
		u = 0;
	}

	unsigned char u;
	signed char s;
};
#endif

/**
 * Avoid implementation-defined underflows during signed to unsigned casts
 */
static inline unsigned ByteToUInt(char c)
{
#if CHAR_MIN == 0
	return c;
#else
	CharS2UConverter converter;

	converter.s = c;
	return converter.u;
#endif
}

/**
 * Compare the expected output with the actual output
 */
static inline bool ComparePackObjectResult(const String& actualOutput, const std::initializer_list<int>& out)
{
	if (actualOutput.GetLength() != out.size())
		return false;

	auto actualOutputPos = actualOutput.Begin();
	for (auto byte : out) {
		if (*actualOutputPos != UIntToByte(byte))
			return false;

		++actualOutputPos;
	}

	return true;
}

/**
 * Pack the given input and compare with the expected output
 */
static inline bool AssertPackObjectResult(Value in, std::initializer_list<int> out)
{
	auto actualOutput = PackObject(in);
	bool equal = ComparePackObjectResult(actualOutput, out);

	if (!equal) {
		std::ostringstream buf;
		buf << std::setw(2) << std::setfill('0') << std::setbase(16);

		buf << "--- ";
		for (int c : out) {
			buf << c;
		}
		buf << std::endl;

		buf << "+++ ";
		for (char c : actualOutput) {
			buf << ByteToUInt(c);
		}
		buf << std::endl;

		BOOST_TEST_MESSAGE(buf.str());
	}

	return equal;
}

BOOST_AUTO_TEST_SUITE(base_object_packer)

BOOST_AUTO_TEST_CASE(pack_null)
{
	BOOST_CHECK(AssertPackObjectResult(Empty, {0}));
}

BOOST_AUTO_TEST_CASE(pack_false)
{
	BOOST_CHECK(AssertPackObjectResult(false, {1}));
}

BOOST_AUTO_TEST_CASE(pack_true)
{
	BOOST_CHECK(AssertPackObjectResult(true, {2}));
}

BOOST_AUTO_TEST_CASE(pack_number)
{
	BOOST_CHECK(AssertPackObjectResult(42.125, {
		// type
		3,
		// IEEE 754
		64, 69, 16, 0, 0, 0, 0, 0
	}));
}

BOOST_AUTO_TEST_CASE(pack_string)
{
	BOOST_CHECK(AssertPackObjectResult(
		String(
			// ASCII (1 to 127)
			"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
			"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
			"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
			"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
			"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
			"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
			"\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
			"\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"
   			// some keyboard-independent non-ASCII unicode characters
			"áéíóú"
		),
		{
			// type
			4,
			// length
			0, 0, 0, 0, 0, 0, 0, 137,
			// ASCII
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
			16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
			64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
			80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
			96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
			112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
			// UTF-8
			195, 161, 195, 169, 195, 173, 195, 179, 195, 186
		}
	));
}

BOOST_AUTO_TEST_CASE(pack_array)
{
	BOOST_CHECK(AssertPackObjectResult(
		(Array::Ptr)new Array({Empty, false, true, 42.125, "foobar"}),
		{
			// type
			5,
			// length
			0, 0, 0, 0, 0, 0, 0, 5,
			// Empty
			0,
			// false
			1,
			// true
			2,
			// 42.125
			3,
			64, 69, 16, 0, 0, 0, 0, 0,
			// "foobar"
			4,
			0, 0, 0, 0, 0, 0, 0, 6,
			102, 111, 111, 98, 97, 114
		}
	));
}

BOOST_AUTO_TEST_CASE(pack_object)
{
	BOOST_CHECK(AssertPackObjectResult(
		(Dictionary::Ptr)new Dictionary({
			{"null", Empty},
			{"false", false},
			{"true", true},
			{"42.125", 42.125},
			{"foobar", "foobar"},
			{"[]", (Array::Ptr)new Array()}
		}),
		{
			// type
			6,
			// length
			0, 0, 0, 0, 0, 0, 0, 6,
			// "42.125"
			0, 0, 0, 0, 0, 0, 0, 6,
			52, 50, 46, 49, 50, 53,
			// 42.125
			3,
			64, 69, 16, 0, 0, 0, 0, 0,
			// "[]"
			0, 0, 0, 0, 0, 0, 0, 2,
			91, 93,
			// (Array::Ptr)new Array()
			5,
			0, 0, 0, 0, 0, 0, 0, 0,
			// "false"
			0, 0, 0, 0, 0, 0, 0, 5,
			102, 97, 108, 115, 101,
			// false
			1,
			// "foobar"
			0, 0, 0, 0, 0, 0, 0, 6,
			102, 111, 111, 98, 97, 114,
			// "foobar"
			4,
			0, 0, 0, 0, 0, 0, 0, 6,
			102, 111, 111, 98, 97, 114,
			// "null"
			0, 0, 0, 0, 0, 0, 0, 4,
			110, 117, 108, 108,
			// Empty
			0,
			// "true"
			0, 0, 0, 0, 0, 0, 0, 4,
			116, 114, 117, 101,
			// true
			2
		}
	));
}

BOOST_AUTO_TEST_SUITE_END()
