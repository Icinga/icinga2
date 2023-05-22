/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/object.hpp"

namespace icinga
{

/**
 * A scoped lock for Objects.
 */
struct ObjectLock
{
public:
	ObjectLock(const Object::Ptr& object);
	ObjectLock(const Object *object);

	ObjectLock(const ObjectLock&) = delete;
	ObjectLock& operator=(const ObjectLock&) = delete;

	~ObjectLock();

	void Lock();
	void Unlock();

private:
	const Object *m_Object{nullptr};
	bool m_Locked{false};
};

}
