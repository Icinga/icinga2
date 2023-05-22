/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include <functional>

namespace icinga
{

/**
 * Priority values for use with the INITIALIZE_ONCE_WITH_PRIORITY macro.
 *
 * The values are given in the order of initialization.
 */
enum class InitializePriority {
	CreateNamespaces,
	InitIcingaApplication,
	RegisterTypeType,
	RegisterObjectType,
	RegisterPrimitiveTypes,
	RegisterBuiltinTypes,
	RegisterFunctions,
	RegisterTypes,
	EvaluateConfigFragments,
	Default,
	FreezeNamespaces,
};

#define I2_TOKENPASTE(x, y) x ## y
#define I2_TOKENPASTE2(x, y) I2_TOKENPASTE(x, y)

#define I2_UNIQUE_NAME(prefix) I2_TOKENPASTE2(prefix, __COUNTER__)

bool InitializeOnceHelper(const std::function<void()>& func, InitializePriority priority = InitializePriority::Default);

#define INITIALIZE_ONCE(func)									\
	namespace { namespace I2_UNIQUE_NAME(io) {							\
		bool l_InitializeOnce(icinga::InitializeOnceHelper(func));		\
	} }

#define INITIALIZE_ONCE_WITH_PRIORITY(func, priority)						\
	namespace { namespace I2_UNIQUE_NAME(io) {							\
		bool l_InitializeOnce(icinga::InitializeOnceHelper(func, priority));	\
	} }
}
