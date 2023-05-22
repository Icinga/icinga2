/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <memory>

namespace icinga
{

#ifndef _WIN32
typedef void *LibraryHandle;
#else /* _WIN32 */
typedef HMODULE LibraryHandle;
#endif /* _WIN32 */

class Library
{
public:
	Library() = default;
	Library(const String& name);

	void *GetSymbolAddress(const String& name) const;

	template<typename T>
	T GetSymbolAddress(const String& name) const
	{
		static_assert(!std::is_same<T, void *>::value, "T must not be void *");

		return reinterpret_cast<T>(GetSymbolAddress(name));
	}

private:
	std::shared_ptr<LibraryHandle> m_Handle;
};

}
