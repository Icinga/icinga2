// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <type_traits>
#include <utility>

namespace icinga
{

template<typename T, typename Enable = void>
struct is_deferrable : std::false_type {};

template<typename T>
struct is_deferrable<std::optional<T>, std::enable_if_t<std::is_invocable_r_v<void, T>>> : std::true_type {};

template<typename T>
struct is_deferrable<T, std::enable_if_t<std::is_invocable_r_v<void, T>>> : std::true_type {};

struct Armed {
    bool value = true;
    Armed() = default;
    explicit Armed(bool enabled) : value{enabled} {};
    Armed(Armed&& o) noexcept : value(o.value) { o.value = false; }
    Armed& operator=(Armed&&) = delete;
};
/**
 * An action to be executed at end of scope.
 *
 * @ingroup base
 */
template<typename DeferredFn, typename = std::enable_if_t<is_deferrable<DeferredFn>::value, int>>
class Defer {
public:
	/**
	 * Construct from either a lambda, a `void(*)()` function pointer or a `std::function<void()>`.
	 *
	 * The type the function gets stored as depends on whether it is explicitly specified or deduced by the CTAD guide
	 * for this constructor.
	 */
	template<typename Fn, std::enable_if_t<std::is_constructible_v<DeferredFn, Fn> && std::is_invocable_r_v<void, Fn>, int> = 0>
	explicit Defer(Fn&& func) : enabled{true}, m_Func(std::forward<Fn>(func))
	{
	}

	Defer() = default;

	/**
	 * Move constructor.
	 *
	 * The exception guarantee is due to move construction of all three supported types also giving the same guarantee.
	 * For lambdas specifically this is guaranteed by the move constructor getting deleted if any of the captured types
	 * move constructors aren't noexcept.
	 */
    Defer(Defer&& other) = default;

	Defer(const Defer&) = delete;
	Defer& operator=(const Defer&) = delete;
	Defer& operator=(Defer&&) = delete;

	~Defer()
	{
		if (enabled.value) {
			try {
				m_Func();
			} catch (...) {
				// https://stackoverflow.com/questions/130117/throwing-exceptions-out-of-a-destructor
			}
		}
	}

	/**
	 * Replace the function executed at the end of the scope with a different one.
	 *
	 * This requires `Defer` to be explicitly templated with a type-erased function type, like
	 * `std::function<void()>` or a `void(*)()` function pointers and won't work with deduced
	 * lambda types.
	 */
	template<typename Fn, std::enable_if_t<std::is_constructible_v<DeferredFn, Fn> && std::is_invocable_r_v<void, Fn>, int> = 0>
	void SetFunc(Fn&& fn)
	{
		m_Func = std::forward<Fn>(fn);
	}

	void Cancel() noexcept
	{
		enabled.value = false;
		// if constexpr (std::is_convertible_v<std::nullptr_t, DeferredFn>) {
		// 	m_Func = nullptr;
		// }
	}

private:
	Armed enabled;
	DeferredFn m_Func{};
};

template<typename Fn, std::enable_if_t<std::is_invocable_r_v<void, Fn>, int> = 0>
Defer(Fn&&) -> Defer<std::decay_t<Fn>>;

} // namespace icinga
