/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_ARRAY_H
#define GENERATOR_ARRAY_H

#include "base/array.hpp"
#include "base/generator.hpp"
#include "base/value.hpp"
#include <cstddef>
#include <utility>

namespace icinga
{

/**
 * A lazy Value supplier of Array items.
 *
 * @ingroup base
 */
class GeneratorArray final : public Generator
{
public:
	inline GeneratorArray(Array::Ptr source)
		: m_Source(std::move(source)), m_Next(0)
	{
	}

	bool GetNext(Value& out) override;

private:
	Array::Ptr m_Source;
	size_t m_Next;
};

}

#endif /* GENERATOR_ARRAY_H */
