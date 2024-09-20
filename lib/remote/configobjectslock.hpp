/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/type.hpp"
#include "base/string.hpp"
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

namespace icinga
{

/**
 * Allows you to easily lock/unlock a specific object of a given type by its name.
 *
 * That way, locking an object "this" of type Host does not affect an object "this" of
 * type "Service" nor an object "other" of type "Host".
 *
 * @ingroup remote
 */
class ObjectNameLock
{
public:
	ObjectNameLock(const Type::Ptr& ptype, const String& objName);

	~ObjectNameLock();

private:
	String m_ObjectName;
	Type::Ptr m_Type;

	static std::mutex m_Mutex;
	static std::condition_variable m_CV;
	static std::map<Type*, std::set<String>> m_LockedObjectNames;
};

}
