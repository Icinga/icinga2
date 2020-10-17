/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_RANGE_H
#define GENERATOR_RANGE_H

#include "base/generator.hpp"
#include "base/value.hpp"

namespace icinga
{

/**
 * Supplies items [start, stop), incrementing by step.
 *
 * @ingroup base
 */
class GeneratorRange final : public Generator
{
public:
	inline GeneratorRange(double start, double stop, double step)
		: m_Current(start), m_Stop(stop), m_Step(step)
	{
	}

	bool GetNext(Value& out) override;

private:
	double m_Current, m_Stop, m_Step;
};

}

#endif /* GENERATOR_RANGE_H */
