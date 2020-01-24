/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_UNIQUE_H
#define GENERATOR_UNIQUE_H

#include "base/generator.hpp"
#include "base/value.hpp"
#include <set>
#include <utility>

namespace icinga
{

/**
 * Filters another generator's unique items.
 *
 * @ingroup base
 */
class GeneratorUnique final : public Generator
{
public:
	inline GeneratorUnique(Generator::Ptr source)
		: m_Source(std::move(source))
	{
	}

	bool GetNext(Value& out) override;

private:
	Generator::Ptr m_Source;
	std::set<Value> m_Unique;
};

}

#endif /* GENERATOR_UNIQUE_H */
