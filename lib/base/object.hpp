/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef OBJECT_H
#define OBJECT_H

#include "base/i2-base.hpp"
#include "base/debug.hpp"
#include <boost/thread/condition_variable.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <vector>

using boost::intrusive_ptr;
using boost::dynamic_pointer_cast;
using boost::static_pointer_cast;

namespace icinga
{

class Value;
class Object;
class Type;
class String;
struct DebugInfo;
class ValidationUtils;

extern Value Empty;

#define DECLARE_PTR_TYPEDEFS(klass) \
	typedef intrusive_ptr<klass> Ptr

#define IMPL_TYPE_LOOKUP_SUPER() 					\

#define IMPL_TYPE_LOOKUP() 							\
	static intrusive_ptr<Type> TypeInstance;				\
	virtual intrusive_ptr<Type> GetReflectionType() const override		\
	{									\
		return TypeInstance;						\
	}

#define DECLARE_OBJECT(klass) \
	DECLARE_PTR_TYPEDEFS(klass); \
	IMPL_TYPE_LOOKUP();

template<typename T>
intrusive_ptr<Object> DefaultObjectFactory(const std::vector<Value>& args)
{
	if (!args.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Constructor does not take any arguments."));

	return new T();
}

template<typename T>
intrusive_ptr<Object> DefaultObjectFactoryVA(const std::vector<Value>& args)
{
	return new T(args);
}

typedef intrusive_ptr<Object> (*ObjectFactory)(const std::vector<Value>&);

template<typename T, bool VA>
struct TypeHelper
{
};

template<typename T>
struct TypeHelper<T, false>
{
	static ObjectFactory GetFactory()
	{
		return DefaultObjectFactory<T>;
	}
};

template<typename T>
struct TypeHelper<T, true>
{
	static ObjectFactory GetFactory()
	{
		return DefaultObjectFactoryVA<T>;
	}
};

/**
 * Base class for all heap-allocated objects. At least one of its methods
 * has to be virtual for RTTI to work.
 *
 * @ingroup base
 */
class Object
{
public:
	DECLARE_PTR_TYPEDEFS(Object);

	Object() = default;
	virtual ~Object();

	virtual String ToString() const;

	virtual intrusive_ptr<Type> GetReflectionType() const;

	virtual void Validate(int types, const ValidationUtils& utils);

	virtual void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty);
	virtual Value GetField(int id) const;
	virtual Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const;
	virtual void SetFieldByName(const String& field, const Value& value, const DebugInfo& debugInfo);
	virtual bool HasOwnField(const String& field) const;
	virtual bool GetOwnField(const String& field, Value *result) const;
	virtual void ValidateField(int id, const Value& value, const ValidationUtils& utils);
	virtual void NotifyField(int id, const Value& cookie = Empty);
	virtual Object::Ptr NavigateField(int id) const;

#ifdef I2_DEBUG
	bool OwnsLock() const;
#endif /* I2_DEBUG */

	static Object::Ptr GetPrototype();

	virtual Object::Ptr Clone() const;

	static intrusive_ptr<Type> TypeInstance;

private:
	Object(const Object& other) = delete;
	Object& operator=(const Object& rhs) = delete;

	uintptr_t m_References{0};
	mutable uintptr_t m_Mutex{0};

#ifdef I2_DEBUG
#	ifndef _WIN32
	mutable pthread_t m_LockOwner;
#	else /* _WIN32 */
	mutable DWORD m_LockOwner;
#	endif /* _WIN32 */
#endif /* I2_DEBUG */

	friend struct ObjectLock;

	friend void intrusive_ptr_add_ref(Object *object);
	friend void intrusive_ptr_release(Object *object);
};

Value GetPrototypeField(const Value& context, const String& field, bool not_found_error, const DebugInfo& debugInfo);

void TypeAddObject(Object *object);
void TypeRemoveObject(Object *object);

void intrusive_ptr_add_ref(Object *object);
void intrusive_ptr_release(Object *object);

template<typename T>
class ObjectImpl
{
};

}

#endif /* OBJECT_H */

#include "base/type.hpp"
