/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_FILTER_H
#define GENERATOR_FILTER_H

#include "base/function.hpp"
#include "base/generator.hpp"
#include "base/value.hpp"
#include <utility>

namespace icinga
{

/**
 * Filters another generator's items based on a function.
 *
 * @ingroup base
 */
class GeneratorFilter final : public Generator
{
public:
	inline GeneratorFilter(Generator::Ptr source, Function::Ptr function)
		: m_Source(std::move(source)), m_Function(std::move(function))
	{
	}

	bool GetNext(Value& out) override;

private:
	Generator::Ptr m_Source;
	Function::Ptr m_Function;
};

}

#endif /* GENERATOR_FILTER_H */
