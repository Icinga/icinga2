/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_MAP_H
#define GENERATOR_MAP_H

#include "base/function.hpp"
#include "base/generator.hpp"
#include "base/value.hpp"
#include <utility>

namespace icinga
{

/**
 * Applies a function to another generator's items.
 *
 * @ingroup base
 */
class GeneratorMap final : public Generator
{
public:
	inline GeneratorMap(Generator::Ptr source, Function::Ptr function)
		: m_Source(std::move(source)), m_Function(std::move(function))
	{
	}

	bool GetNext(Value& out) override;

private:
	Generator::Ptr m_Source;
	Function::Ptr m_Function;
};

}

#endif /* GENERATOR_MAP_H */
