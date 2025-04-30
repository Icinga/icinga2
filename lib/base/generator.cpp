/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/array.hpp"
#include "base/generator.hpp"
#include "base/generator-unique.hpp"
#include "base/objectlock.hpp"
#include "base/primitivetype.hpp"
#include <algorithm>
#include <utility>

using namespace icinga;

REGISTER_PRIMITIVE_TYPE(Generator, Object, Generator::GetPrototype());

bool Generator::GetNext(Value& out)
{
	return false;
}

/**
 * Counts the items.
 *
 * @returns Amount of items.
 */
size_t Generator::GetLength()
{
	size_t len = 0;
	Value buf;

	while (GetNext(buf)) {
		++len;
	}

	return len;
}

/**
 * Checks whether the items include the specified value.
 *
 * @param value The value.
 * @returns true if the items include the value, false otherwise.
 */
bool Generator::Contains(const Value& value)
{
	Value buf;

	while (GetNext(buf)) {
		if (buf == value) {
			return true;
		}
	}

	return false;
}

Array::Ptr Generator::Reverse()
{
	auto result (ToArray());
	ObjectLock oLock (result);

	std::reverse(result->Begin(), result->End());

	return std::move(result);
}

Value Generator::Join(const Value& separator)
{
	Value result;
	Value buf;
	bool first = true;

	while (GetNext(buf)) {
		if (first) {
			first = false;
		} else {
			result = std::move(result) + separator;
		}

		result = std::move(result) + std::move(buf);
	}

	return std::move(result);
}

Generator::Ptr Generator::Unique()
{
	return new GeneratorUnique(this);
}

Array::Ptr Generator::ToArray()
{
	Array::Ptr result (new Array());
	Value buf;

	while (GetNext(buf)) {
		result->Add(std::move(buf));
	}

	return std::move(result);
}
