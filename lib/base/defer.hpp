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
struct is_deferrable<std::optional<T>, std::enable_if_t<std::is_void_v<std::invoke_result_t<T>>>> : std::true_type {};

template<typename T>
struct is_deferrable<T, std::enable_if_t<std::is_void_v<std::invoke_result_t<T>>>> : std::true_type {};

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
	template<typename Fn, std::enable_if_t<std::is_constructible_v<DeferredFn, Fn>, int> = 0>
	explicit Defer(Fn&& func) : m_Func(std::forward<Fn>(func))
	{
	}

	/**
	 * Default constructor.
	 *
	 * Default construction will only be possible if this class is templated with a type erased function type, like
	 * std::function<void()> or void(*)().
	 */
	Defer() = default;

	/**
	 * Move constructor.
	 *
	 * Only participates in overload resolution if DeferredFn's move constructor is noexcept. A throwing move
	 * could leave `other` holding a moved-from functor without `*this` having taken over the deferred action.
	 */
	template<typename T = DeferredFn, std::enable_if_t<std::is_nothrow_move_constructible_v<T>, int> = 0>
	Defer(Defer&& other) noexcept : m_Func(std::move(other).m_Func)
	{
		other.Cancel();
	}

	Defer(const Defer&) = delete;
	Defer& operator=(const Defer&) = delete;
	Defer& operator=(Defer&&) = delete;

	~Defer()
	{
		if (m_Func) {
			try {
				if constexpr (std::is_convertible_v<std::nullopt_t, DeferredFn>) {
					(*m_Func)();
				} else {
					m_Func();
				}
			} catch (...) {
				// https://stackoverflow.com/questions/130117/throwing-exceptions-out-of-a-destructor
			}
		}
	}

	/**
	 * Replace the function executed at the end of the scope with a different one.
	 *
	 * This requires `Defer` to be explicitly templated with a type-erased function type, like `std::function<void()>`
	 * or a `void(*)()` function pointers and won't work with deduced lambda types.
	 */
	template<typename Fn, std::enable_if_t<std::is_constructible_v<DeferredFn, Fn>, int> = 0>
	void SetFunc(Fn&& fn)
	{
		m_Func = std::forward<Fn>(fn);
	}

	void Cancel() noexcept
	{
		if constexpr (std::is_convertible_v<std::nullopt_t, DeferredFn>) {
			m_Func.reset();
		} else {
			m_Func = nullptr;
		}
	}

private:
	DeferredFn m_Func{};
};

template<typename Fn>
Defer(Fn&&) -> Defer<std::optional<std::decay_t<Fn>>>;

} // namespace icinga
