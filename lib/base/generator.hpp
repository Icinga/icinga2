// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include <optional>

namespace icinga
{

/**
 * Abstract base class for generators that produce a sequence of Values.
 *
 * @note Any instance of a Generator should be treated as an @c Array -like object that produces its elements
 * on-the-fly.
 *
 * @ingroup base
 */
class Generator : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Generator);

	/**
	 * Produces the next Value in the sequence.
	 *
	 * This method returns the next Value produced by the generator. If the generator is exhausted,
	 * it returns std::nullopt for all subsequent calls to this method.
	 *
	 * @return The next Value in the sequence, or std::nullopt if the generator is exhausted.
	 */
	virtual std::optional<Value> Next() = 0;
};

/**
 * A generator that transforms elements of a container into Values using a provided transformation function.
 *
 * This class takes a container and a transformation function as input. It uses the transformation function
 * to convert each element of the container into a Value. The generator produces Values on-the-fly as they
 * are requested via the `Next()` method.
 *
 * @tparam Container The type of the container holding the elements to be transformed.
 *
 * @ingroup base
 */
template<typename Container>
class ValueGenerator final : public Generator
{
public:
	DECLARE_PTR_TYPEDEFS(ValueGenerator);

	// The type of elements in the container and the type to be passed to the transformation function.
	using InputType = typename Container::value_type;

	// The type of the transformation function that takes an element of the container and returns a Value.
	using TransformFunc = std::function<Value (const InputType&)>;

	/**
	 * Constructs a ValueGenerator with the given container and transformation function.
	 *
	 * The generator will iterate over the elements of the container, applying the transformation function
	 * to each element to produce values on-the-fly. You must ensure that the container remains valid for
	 * the lifetime of the generator.
	 *
	 * @param container The container holding the elements to be transformed.
	 * @param generator The transformation function to convert elements into Values.
	 */
	ValueGenerator(Container& container, TransformFunc generator)
		: m_It{container.begin()}, m_End{container.end()}, m_Func{std::move(generator)}
	{
	}

	std::optional<Value> Next() override
	{
		if (m_It == m_End) {
			return std::nullopt;
		}

		auto next = m_Func(*m_It);
		++m_It;
		return next;
	}

private:
	using Iterator = typename Container::iterator;
	Iterator m_It; // Current iterator position.
	Iterator m_End; // End iterator position.

	TransformFunc m_Func; // The transformation function.
};

} // namespace icinga
