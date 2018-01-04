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

#include "base/object.hpp"
#include "base/value.hpp"
#include "base/dictionary.hpp"
#include "base/primitivetype.hpp"
#include "base/utility.hpp"
#include "base/timer.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/thread/recursive_mutex.hpp>

using namespace icinga;

DEFINE_TYPE_INSTANCE(Object);

#ifdef I2_LEAK_DEBUG
static boost::mutex l_ObjectCountLock;
static std::map<String, int> l_ObjectCounts;
static Timer::Ptr l_ObjectCountTimer;
#endif /* I2_LEAK_DEBUG */

/**
 * Destructor for the Object class.
 */
Object::~Object()
{
	delete reinterpret_cast<boost::recursive_mutex *>(m_Mutex);
}

/**
 * Returns a string representation for the object.
 */
String Object::ToString() const
{
	return "Object of type '" + GetReflectionType()->GetName() + "'";
}

#ifdef I2_DEBUG
/**
 * Checks if the calling thread owns the lock on this object.
 *
 * @returns True if the calling thread owns the lock, false otherwise.
 */
bool Object::OwnsLock() const
{
#ifdef _WIN32
	DWORD tid = InterlockedExchangeAdd(&m_LockOwner, 0);

	return (tid == GetCurrentThreadId());
#else /* _WIN32 */
	pthread_t tid = __sync_fetch_and_add(&m_LockOwner, nullptr);

	return (tid == pthread_self());
#endif /* _WIN32 */
}
#endif /* I2_DEBUG */

void Object::SetField(int id, const Value&, bool, const Value&)
{
	if (id == 0)
		BOOST_THROW_EXCEPTION(std::runtime_error("Type field cannot be set."));
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

Value Object::GetField(int id) const
{
	if (id == 0)
		return GetReflectionType()->GetName();
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

bool Object::HasOwnField(const String& field) const
{
	Type::Ptr type = GetReflectionType();

	if (!type)
		return false;

	return type->GetFieldId(field) != -1;
}

bool Object::GetOwnField(const String& field, Value *result) const
{
	Type::Ptr type = GetReflectionType();

	if (!type)
		return false;

	int tid = type->GetFieldId(field);

	if (tid == -1)
		return false;

	*result = GetField(tid);
	return true;
}

Value Object::GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const
{
	Type::Ptr type = GetReflectionType();

	if (!type)
		return Empty;

	int fid = type->GetFieldId(field);

	if (fid == -1)
		return GetPrototypeField(const_cast<Object *>(this), field, true, debugInfo);

	if (sandboxed) {
		Field fieldInfo = type->GetFieldInfo(fid);

		if (fieldInfo.Attributes & FANoUserView)
			BOOST_THROW_EXCEPTION(ScriptError("Accessing the field '" + field + "' for type '" + type->GetName() + "' is not allowed in sandbox mode.", debugInfo));
	}

	return GetField(fid);
}

void Object::SetFieldByName(const String& field, const Value& value, const DebugInfo& debugInfo)
{
	Type::Ptr type = GetReflectionType();

	if (!type)
		BOOST_THROW_EXCEPTION(ScriptError("Cannot set field on object.", debugInfo));

	int fid = type->GetFieldId(field);

	if (fid == -1)
		BOOST_THROW_EXCEPTION(ScriptError("Attribute '" + field + "' does not exist.", debugInfo));

	try {
		SetField(fid, value);
	} catch (const boost::bad_lexical_cast&) {
		Field fieldInfo = type->GetFieldInfo(fid);
		Type::Ptr ftype = Type::GetByName(fieldInfo.TypeName);
		BOOST_THROW_EXCEPTION(ScriptError("Attribute '" + field + "' cannot be set to value of type '" + value.GetTypeName() + "', expected '" + ftype->GetName() + "'", debugInfo));
	} catch (const std::bad_cast&) {
		Field fieldInfo = type->GetFieldInfo(fid);
		Type::Ptr ftype = Type::GetByName(fieldInfo.TypeName);
		BOOST_THROW_EXCEPTION(ScriptError("Attribute '" + field + "' cannot be set to value of type '" + value.GetTypeName() + "', expected '" + ftype->GetName() + "'", debugInfo));
	}
}

void Object::Validate(int types, const ValidationUtils& utils)
{
	/* Nothing to do here. */
}

void Object::ValidateField(int id, const Value& value, const ValidationUtils& utils)
{
	/* Nothing to do here. */
}

void Object::NotifyField(int id, const Value& cookie)
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

Object::Ptr Object::NavigateField(int id) const
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

Object::Ptr Object::Clone() const
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Object cannot be cloned."));
}

Type::Ptr Object::GetReflectionType() const
{
	return Object::TypeInstance;
}

Value icinga::GetPrototypeField(const Value& context, const String& field, bool not_found_error, const DebugInfo& debugInfo)
{
	Type::Ptr ctype = context.GetReflectionType();
	Type::Ptr type = ctype;

	do {
		Object::Ptr object = type->GetPrototype();

		if (object && object->HasOwnField(field))
			return object->GetFieldByName(field, false, debugInfo);

		type = type->GetBaseType();
	} while (type);

	if (not_found_error)
		BOOST_THROW_EXCEPTION(ScriptError("Invalid field access (for value of type '" + ctype->GetName() + "'): '" + field + "'", debugInfo));
	else
		return Empty;
}

#ifdef I2_LEAK_DEBUG
void icinga::TypeAddObject(Object *object)
{
	boost::mutex::scoped_lock lock(l_ObjectCountLock);
	String typeName = Utility::GetTypeName(typeid(*object));
	l_ObjectCounts[typeName]++;
}

void icinga::TypeRemoveObject(Object *object)
{
	boost::mutex::scoped_lock lock(l_ObjectCountLock);
	String typeName = Utility::GetTypeName(typeid(*object));
	l_ObjectCounts[typeName]--;
}

static void TypeInfoTimerHandler()
{
	boost::mutex::scoped_lock lock(l_ObjectCountLock);

	typedef std::map<String, int>::value_type kv_pair;
	for (kv_pair& kv : l_ObjectCounts) {
		if (kv.second == 0)
			continue;

		Log(LogInformation, "TypeInfo")
			<< kv.second << " " << kv.first << " objects";

		kv.second = 0;
	}
}

INITIALIZE_ONCE([]() {
	l_ObjectCountTimer = new Timer();
	l_ObjectCountTimer->SetInterval(10);
	l_ObjectCountTimer->OnTimerExpired.connect(std::bind(TypeInfoTimerHandler));
	l_ObjectCountTimer->Start();
});
#endif /* I2_LEAK_DEBUG */

void icinga::intrusive_ptr_add_ref(Object *object)
{
#ifdef I2_LEAK_DEBUG
	if (object->m_References == 0)
		TypeAddObject(object);
#endif /* I2_LEAK_DEBUG */

#ifdef _WIN32
	InterlockedIncrement(&object->m_References);
#else /* _WIN32 */
	__sync_add_and_fetch(&object->m_References, 1);
#endif /* _WIN32 */
}

void icinga::intrusive_ptr_release(Object *object)
{
	uintptr_t refs;

#ifdef _WIN32
	refs = InterlockedDecrement(&object->m_References);
#else /* _WIN32 */
	refs = __sync_sub_and_fetch(&object->m_References, 1);
#endif /* _WIN32 */

	if (unlikely(refs == 0)) {
#ifdef I2_LEAK_DEBUG
		TypeRemoveObject(object);
#endif /* I2_LEAK_DEBUG */

		delete object;
	}
}
