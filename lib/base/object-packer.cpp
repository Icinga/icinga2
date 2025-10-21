/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/object-packer.hpp"
#include "base/debug.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>
#include <utility>

using namespace icinga;

union EndiannessDetector
{
	EndiannessDetector()
	{
		i = 1;
	}

	int i;
	char buf[sizeof(int)];
};

static const EndiannessDetector l_EndiannessDetector;

// Assumption: The compiler will optimize (away) if/else statements using this.
#define MACHINE_LITTLE_ENDIAN (l_EndiannessDetector.buf[0])

static void PackAny(const Value& value, std::string& builder);

/**
 * std::swap() seems not to work
 */
static inline void SwapBytes(char& a, char& b)
{
	char c = a;
	a = b;
	b = c;
}

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

/**
 * Append the given int as big-endian 64-bit unsigned int
 */
static inline void PackUInt64BE(uint_least64_t i, std::string& builder)
{
	char buf[8] = {
		UIntToByte(i >> 56u),
		UIntToByte((i >> 48u) & 255u),
		UIntToByte((i >> 40u) & 255u),
		UIntToByte((i >> 32u) & 255u),
		UIntToByte((i >> 24u) & 255u),
		UIntToByte((i >> 16u) & 255u),
		UIntToByte((i >> 8u) & 255u),
		UIntToByte(i & 255u)
	};

	builder.append((char*)buf, 8);
}

union Double2BytesConverter
{
	Double2BytesConverter()
	{
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = 0;
		buf[7] = 0;
	}

	double f;
	char buf[8];
};

/**
 * Append the given double as big-endian IEEE 754 binary64
 */
static inline void PackFloat64BE(double f, std::string& builder)
{
	Double2BytesConverter converter;

	converter.f = f;

	if (MACHINE_LITTLE_ENDIAN) {
		SwapBytes(converter.buf[0], converter.buf[7]);
		SwapBytes(converter.buf[1], converter.buf[6]);
		SwapBytes(converter.buf[2], converter.buf[5]);
		SwapBytes(converter.buf[3], converter.buf[4]);
	}

	builder.append((char*)converter.buf, 8);
}

/**
 * Append the given string's length (BE uint64) and the string itself
 */
static inline void PackString(const String& string, std::string& builder)
{
	PackUInt64BE(string.GetLength(), builder);
	builder += string.GetData();
}

/**
 * Append the given array
 */
static inline void PackArray(const Array::Ptr& arr, std::string& builder)
{
	ObjectLock olock(arr);

	builder += '\5';
	PackUInt64BE(arr->GetLength(), builder);

	for (const Value& value : arr) {
		PackAny(value, builder);
	}
}

/**
 * Append the given dictionary
 */
static inline void PackDictionary(const Dictionary::Ptr& dict, std::string& builder)
{
	ObjectLock olock(dict);

	builder += '\6';
	PackUInt64BE(dict->GetLength(), builder);

	for (const Dictionary::Pair& kv : dict) {
		PackString(kv.first, builder);
		PackAny(kv.second, builder);
	}
}

/**
 * Append any JSON-encodable value
 */
static void PackAny(const Value& value, std::string& builder)
{
	switch (value.GetType()) {
		case ValueString:
			builder += '\4';
			PackString(value.Get<String>(), builder);
			break;

		case ValueNumber:
			builder += '\3';
			PackFloat64BE(value.Get<double>(), builder);
			break;

		case ValueBoolean:
			builder += (value.ToBool() ? '\2' : '\1');
			break;

		case ValueEmpty:
			builder += '\0';
			break;

		case ValueObject:
			{
				const Object::Ptr& obj = value.Get<Object::Ptr>();

				Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(obj);
				if (dict) {
					PackDictionary(dict, builder);
					break;
				}

				Array::Ptr arr = dynamic_pointer_cast<Array>(obj);
				if (arr) {
					PackArray(arr, builder);
					break;
				}
			}

			builder += '\0';
			break;

		default:
			VERIFY(!"Invalid variant type.");
	}
}

/**
 * Pack any JSON-encodable value to a BSON-similar structure suitable for consistent hashing
 *
 * Spec:
 *   null: 0x00
 *   false: 0x01
 *   true: 0x02
 *   number: 0x03 (ieee754_binary64_bigendian)payload
 *   string: 0x04 (uint64_bigendian)payload.length (char[])payload
 *   array: 0x05 (uint64_bigendian)payload.length (any[])payload
 *   object: 0x06 (uint64_bigendian)payload.length (keyvalue[])payload.sort()
 *
 *   any: null|false|true|number|string|array|object
 *   keyvalue: (uint64_bigendian)key.length (char[])key (any)value
 *
 * Assumptions:
 *   - double is IEEE 754 binary64
 *   - all int types (signed and unsigned) and all float types share the same endianness
 *   - char is exactly 8 bits wide and one char is exactly one byte affected by the machine endianness
 *   - all input strings, arrays and dictionaries are at most 2^64-1 long
 *
 * If not, this function will silently produce invalid results.
 */
String icinga::PackObject(const Value& value)
{
	std::string builder;
	PackAny(value, builder);

	return builder;
}
