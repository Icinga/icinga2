/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_LOCAL_H
#define UT_LOCAL_H

#include "base/shared-object.hpp"
#include "base/ut-current.hpp"
#include <unordered_map>

namespace icinga
{
namespace UT
{

extern thread_local std::unordered_map<void*, SharedObject::Ptr> l_KernelspaceThreadLocals;

/**
 * A UT::Thread-local variable.
 *
 * @ingroup base
 */
template<class T>
class Local
{
public:
	inline Local() = default;

	Local(const Local&) = delete;
	Local(Local&&) = delete;
	Local& operator=(const Local&) = delete;
	Local& operator=(Local&&) = delete;

	inline T& operator*()
	{
		return Get();
	}

	inline T* operator->()
	{
		return &Get();
	}

private:
	class Storage;

	T& Get()
	{
		auto locals (UT::Current::m_Thread == nullptr ? &UT::l_KernelspaceThreadLocals : &UT::Current::m_Thread->m_Locals);
		auto& storage ((*locals)[this]);

		if (!storage) {
			storage = new Storage();
		}

		return static_cast<Storage*>(storage.get())->Var;
	}
};

/**
 * Storage for a UT::Thread-local variable.
 *
 * @ingroup base
 */
template<class T>
class Local<T>::Storage : public SharedObject
{
public:
	T Var;
};

}
}

#endif /* UT_LOCAL_H */
