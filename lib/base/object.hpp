/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OBJECT_H
#define OBJECT_H

#include "base/i2-base.hpp"
#include "base/debug.hpp"
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
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

#define REQUIRE_NOT_NULL(ptr) RequireNotNullInternal(ptr, #ptr)

void RequireNotNullInternal(const intrusive_ptr<Object>& object, const char *description);

void DefaultObjectFactoryCheckArgs(const std::vector<Value>& args);

template<typename T>
intrusive_ptr<Object> DefaultObjectFactory(const std::vector<Value>& args)
{
	DefaultObjectFactoryCheckArgs(args);

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

template<typename T>
struct Lazy
{
	using Accessor = std::function<T ()>;

	explicit Lazy(T value)
		: m_Cached(true), m_Value(value)
 	{ }

	explicit Lazy(Accessor accessor)
		: m_Accessor(accessor)
	{ }

	template<typename U>
	explicit Lazy(const Lazy<U>& other)
	{
		if (other.m_Cached) {
			m_Accessor = Accessor();
			m_Value = static_cast<T>(other.m_Value);
			m_Cached = true;
		} else {
			auto accessor = other.m_Accessor;
			m_Accessor = [accessor]() { return static_cast<T>(accessor()); };
			m_Cached = false;
		}
	}

	template<typename U>
	operator Lazy<U>() const
	{
		if (m_Cached)
			return Lazy<U>(static_cast<U>(m_Value));
		else {
			Accessor accessor = m_Accessor;
			return Lazy<U>(static_cast<typename Lazy<U>::Accessor>([accessor]() { return static_cast<U>(accessor()); }));
		}
	}

	const T& operator()() const
	{
		if (!m_Cached) {
			m_Value = m_Accessor();
			m_Cached = true;
		}

		return m_Value;
	}

private:
	Accessor m_Accessor;
	mutable bool m_Cached{false};
	mutable T m_Value;

	template<typename U>
	friend struct Lazy;
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

	Object();
	virtual ~Object();

	virtual String ToString() const;

	virtual intrusive_ptr<Type> GetReflectionType() const;

	virtual void Validate(int types, const ValidationUtils& utils);

	virtual void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty);
	virtual Value GetField(int id) const;
	virtual Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const;
	virtual void SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo);
	virtual bool HasOwnField(const String& field) const;
	virtual bool GetOwnField(const String& field, Value *result) const;
	virtual void ValidateField(int id, const Lazy<Value>& lvalue, const ValidationUtils& utils);
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

	std::atomic<uint_fast64_t> m_References;
	mutable uintptr_t m_Mutex{0};

#ifdef I2_DEBUG
#	ifndef _WIN32
	mutable pthread_t m_LockOwner;
#	else /* _WIN32 */
	mutable DWORD m_LockOwner;
#	endif /* _WIN32 */
	mutable size_t m_LockCount = 0;
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
