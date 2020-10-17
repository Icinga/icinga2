/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "base/array.hpp"
#include "base/object.hpp"
#include "base/value.hpp"
#include <cstddef>

namespace icinga
{

/**
 * A lazy Value items supplier.
 *
 * @ingroup base
 */
class Generator : public Object
{
public:
	DECLARE_OBJECT(Generator);

	static Object::Ptr GetPrototype();

	Generator() = default;

	Generator(const Generator&) = delete;
	Generator(Generator&&) = delete;
	Generator& operator=(const Generator&) = delete;
	Generator& operator=(Generator&&) = delete;

	virtual ~Generator() = default;

	virtual bool GetNext(Value& out);

	size_t GetLength();
	bool Contains(const Value& value);
	Array::Ptr Reverse();
	Value Join(const Value& separator);
	Generator::Ptr Unique();
	Array::Ptr ToArray();
};

}

#endif /* GENERATOR_H */
