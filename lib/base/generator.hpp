// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include <optional>

namespace icinga
{

/**
 * ValueGenerator is a class that defines a generator function type for producing Values on demand.
 *
 * This class is used to create generator functions that can yield any values that can be represented by the
 * Icinga Value type. The generator function is exhausted when it returns `std::nullopt`, indicating that there
 * are no more values to produce. Subsequent calls to `Next()` will always return `std::nullopt` after exhaustion.
 *
 * @ingroup base
 */
class ValueGenerator final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ValueGenerator);

	/**
	 * Generates a Value using the provided generator function.
	 *
	 * The generator function should return an `std::optional<Value>` which contains the produced Value or
	 * `std::nullopt` when there are no more values to produce. After the generator function returns `std::nullopt`,
	 * the generator is considered exhausted, and further calls to `Next()` will always return `std::nullopt`.
	 */
	using GenFunc = std::function<std::optional<Value>()>;

	explicit ValueGenerator(GenFunc generator): m_Generator(std::move(generator))
	{
	}

	std::optional<Value> Next() const
	{
		return m_Generator();
	}

private:
	GenFunc m_Generator; // The generator function that produces Values.
};

}
