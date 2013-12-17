/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef DYNAMICOBJECT_H
#define DYNAMICOBJECT_H

#include "base/i2-base.h"
#include "base/dynamicobject.th"
#include "base/object.h"
#include "base/dictionary.h"
#include "base/array.h"
#include <boost/signals2.hpp>
#include <map>
#include <set>

namespace icinga
{

class DynamicType;

enum DomainPriv
{
	DomainPrivRead = (1<<0),
	DomainPrivCheckResult = (1<<1),
	DomainPrivCommand = (1<<2)
};

/**
 * A dynamic object that can be instantiated from the configuration file
 * and that supports attribute replication to remote application instances.
 *
 * @ingroup base
 */
class I2_BASE_API DynamicObject : public ObjectImpl<DynamicObject>
{
public:
	DECLARE_PTR_TYPEDEFS(DynamicObject);

	static void StaticInitialize(void);

	static boost::signals2::signal<void (const DynamicObject::Ptr&)> OnStarted;
	static boost::signals2::signal<void (const DynamicObject::Ptr&)> OnStopped;
	static boost::signals2::signal<void (const DynamicObject::Ptr&)> OnStateChanged;
	static boost::signals2::signal<void (const DynamicObject::Ptr&, const String&, bool)> OnAuthorityChanged;

	Value InvokeMethod(const String& method, const std::vector<Value>& arguments);

	shared_ptr<DynamicType> GetType(void) const;

	bool IsActive(void) const;

	void SetAuthority(const String& type, bool value);
	bool HasAuthority(const String& type) const;

	void SetPrivileges(const String& instance, int privs);
	bool HasPrivileges(const String& instance, int privs) const;

	void SetExtension(const String& key, const Object::Ptr& object);
	Object::Ptr GetExtension(const String& key);
	void ClearExtension(const String& key);

	void Register(void);

	void Activate(void);
	void Deactivate(void);

	virtual void Start(void);
	virtual void Stop(void);

	virtual void OnConfigLoaded(void);
	virtual void OnStateLoaded(void);

	template<typename T>
	static shared_ptr<T> GetObject(const String& name)
	{
		DynamicObject::Ptr object = GetObject(T::GetTypeName(), name);

		return static_pointer_cast<T>(object);
	}

	static void DumpObjects(const String& filename, int attributeTypes = FAState);
	static void RestoreObjects(const String& filename, int attributeTypes = FAState);
	static void StopObjects(void);

protected:
	explicit DynamicObject(void);

private:
	std::map<String, int> m_Privileges;

	static DynamicObject::Ptr GetObject(const String& type, const String& name);
};

#define DECLARE_TYPENAME(klass)						\
	inline static String GetTypeName(void)				\
	{								\
		return #klass;						\
	}								\
									\
	inline static shared_ptr<klass> GetByName(const String& name)	\
	{								\
		return DynamicObject::GetObject<klass>(name);		\
	}

}

#endif /* DYNAMICOBJECT_H */
