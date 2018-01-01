/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include <boost/thread/recursive_mutex.hpp>
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

extern I2_BASE_API Value Empty;

#define DECLARE_PTR_TYPEDEFS(klass) \
	typedef intrusive_ptr<klass> Ptr

#define IMPL_TYPE_LOOKUP_SUPER() 					\

#define IMPL_TYPE_LOOKUP() 							\
	static intrusive_ptr<Type> TypeInstance;				\
	virtual intrusive_ptr<Type> GetReflectionType(void) const override	\
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
	static ObjectFactory GetFactory(void)
	{
		return DefaultObjectFactory<T>;
	}
};

template<typename T>
struct TypeHelper<T, true>
{
	static ObjectFactory GetFactory(void)
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
class I2_BASE_API Object
{
public:
	DECLARE_PTR_TYPEDEFS(Object);

	Object(void);
	virtual ~Object(void);

	virtual String ToString(void) const;

	virtual intrusive_ptr<Type> GetReflectionType(void) const;

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
	bool OwnsLock(void) const;
#endif /* I2_DEBUG */

	static Object::Ptr GetPrototype(void);

	virtual Object::Ptr Clone(void) const;

	static intrusive_ptr<Type> TypeInstance;

private:
	Object(const Object& other);
	Object& operator=(const Object& rhs);

	std::atomic<uintptr_t> m_References;
	mutable std::atomic<uintptr_t> m_Mutex;

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

I2_BASE_API Value GetPrototypeField(const Value& context, const String& field, bool not_found_error, const DebugInfo& debugInfo);

I2_BASE_API void TypeAddObject(Object *object);
I2_BASE_API void TypeRemoveObject(Object *object);

inline void intrusive_ptr_add_ref(Object *object)
{
	uintptr_t refs = object->m_References.fetch_add(std::memory_order_relaxed);

#ifdef I2_LEAK_DEBUG
	if (refs == 1)
		TypeAddObject(object);
#endif /* I2_LEAK_DEBUG */
}

inline void intrusive_ptr_release(Object *object)
{
	uintptr_t refs = object->m_References.fetch_sub(1, std::memory_order_release);

	if (unlikely(refs == 1)) {
		std::atomic_thread_fence(std::memory_order_acquire);

#ifdef I2_LEAK_DEBUG
		TypeRemoveObject(object);
#endif /* I2_LEAK_DEBUG */

		delete object;
	}
}

template<typename T>
class ObjectImpl
{
};

}

#endif /* OBJECT_H */

#include "base/type.hpp"
