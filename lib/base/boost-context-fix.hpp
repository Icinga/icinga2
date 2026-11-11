// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <exception>

/* Repeating the boost version range here mostly for documentation, since it's already included
 * conditionally via CMakeLists.txt.
 * This also checks for the unaffected alternate Boost.Fiber implementations from the selection in
 * `boost/context/fiber.hpp`, since it's only required with the default case of "fcontext".
 */
#if BOOST_VERSION >= 108800 && BOOST_VERSION < 109200 && defined(__GLIBCXX__) && !defined(BOOST_USE_UCONTEXT) && \
	!defined(BOOST_USE_WINFIB)

#ifdef BOOST_CONTEXT_FIBER_H
#error "boost/context/fiber.hpp has been included before base/boost-context-fix.hpp"
#endif /* BOOST_CONTEXT_FIBER_H */

#include <cxxabi.h>

namespace icinga
{
namespace workaround
{

/* Work around https://github.com/boostorg/context/issues/323 (Boost 1.88 - 1.91 with libstdc++).
 *
 * boost::context::fiber saves the per-thread C++ exception state before every context switch and
 * restores it after, both via __cxa_get_globals() which libstdc++ declares as __attribute__((const)).
 * So the compiler may call it only once and cache the result until invalidated. And since the compiler
 * is not aware of Boost's context switches it will not invalidate the cached result once a coroutine
 * resumes on a different thread, which leads to the coroutine overwriting the other thread's exception
 * state.
 *
 * Redirecting these calls to this function instead that cannot be inlined (`BOOST_NOINLINE`), cached
 * (non-const) or optimized away via IPA (asm memory clobber) solves this.
 */
BOOST_NOINLINE inline __cxxabiv1::__cxa_eh_globals* GetCxaEhGlobalsUncached() noexcept
{
	__asm__ __volatile__("" ::: "memory");
	return __cxxabiv1::__cxa_get_globals();
}

} // namespace workaround
} // namespace icinga

/* Redifines the problematic function call while the fiber.hpp header is being included.
 * This is ugly, but should be safe and reliable here due to the scope being limited to this
 * one header in a limited set of versions.
 */
#define __cxa_get_globals ::icinga::workaround::GetCxaEhGlobalsUncached
#include <boost/context/fiber.hpp>
#undef __cxa_get_globals

#endif
