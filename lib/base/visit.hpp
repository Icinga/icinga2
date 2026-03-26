// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <variant>

namespace icinga {

namespace detail{
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<std::remove_reference_t<Ts>...>;
} // namespace detail

/**
 * A more convenient wrapper around std::visit.
 *
 * Instead of allowing to dispatch a single functor on an arbitrary number of variant objects,
 * this allows to dispatch any number of overloads on a single variant object, which more closely
 * fits the way this project uses @c std::variant.
 *
 * @param variant The variant to dispatch on
 * @param overloads A set of overloads to handle each member of the variant
 *
 * @return The value std::visit() returns.
 */
template<typename Variant, typename... Overloads>
auto Visit(Variant&& variant, Overloads&&... overloads)
{
	return std::visit(detail::Overloaded{std::forward<Overloads>(overloads)...}, std::forward<Variant>(variant));
}

} // namespace icinga
