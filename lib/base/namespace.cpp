// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/namespace.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/debuginfo.hpp"
#include "base/exception.hpp"
#include <sstream>

using namespace icinga;

template class std::map<icinga::String, std::shared_ptr<icinga::NamespaceValue> >;

REGISTER_PRIMITIVE_TYPE(Namespace, Object, Namespace::GetPrototype());

/**
 * Creates a new namespace.
 *
 * @param constValues If true, all values inserted into the namespace are treated as constants and can't be updated.
 */
Namespace::Namespace(bool constValues)
	: m_ConstValues(constValues), m_Frozen(false)
{ }

Value Namespace::Get(const String& field) const
{
	Value value;
	if (!Get(field, &value))
		BOOST_THROW_EXCEPTION(ScriptError("Namespace does not contain field '" + field + "'"));
	return value;
}

bool Namespace::Get(const String& field, Value *value) const
{
	auto lock(ReadLockUnlessFrozen());

	auto nsVal = m_Data.find(field);

	if (nsVal == m_Data.end()) {
		return false;
	}

	*value = nsVal->second.Val;
	return true;
}

void Namespace::Set(const String& field, const Value& value, bool isConst, const DebugInfo& debugInfo)
{
	ObjectLock olock(this);

	if (m_Frozen) {
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified.", debugInfo));
	}

	std::unique_lock<decltype(m_DataMutex)> dlock (m_DataMutex);

	auto nsVal = m_Data.find(field);

	if (nsVal == m_Data.end()) {
		m_Data[field] = NamespaceValue{value, isConst || m_ConstValues};
	} else {
		if (nsVal->second.Const) {
			BOOST_THROW_EXCEPTION(ScriptError("Constant must not be modified.", debugInfo));
		}

		nsVal->second.Val = value;
	}
}

/**
 * Returns the number of elements in the namespace.
 *
 * @returns Number of elements.
 */
size_t Namespace::GetLength() const
{
	auto lock(ReadLockUnlessFrozen());

	return m_Data.size();
}

bool Namespace::Contains(const String& field) const
{
	auto lock (ReadLockUnlessFrozen());

	return m_Data.find(field) != m_Data.end();
}

void Namespace::Remove(const String& field)
{
	ObjectLock olock(this);

	if (m_Frozen) {
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified."));
	}

	std::unique_lock<decltype(m_DataMutex)> dlock (m_DataMutex);

	auto it = m_Data.find(field);

	if (it == m_Data.end()) {
		return;
	}

	if (it->second.Const) {
		BOOST_THROW_EXCEPTION(ScriptError("Constants must not be removed."));
	}

	m_Data.erase(it);
}

/**
 * Freeze the namespace, preventing further updates.
 *
 * This only prevents inserting, replacing or deleting values from the namespace. This operation has no effect on
 * objects referenced by the values, these remain mutable if they were before.
 */
void Namespace::Freeze() {
	ObjectLock olock(this);

	m_Frozen.store(true, std::memory_order_release);
}

bool Namespace::Frozen() const
{
	return m_Frozen.load(std::memory_order_acquire);
}

/**
 * Returns an already locked ObjectLock if the namespace is frozen.
 * Otherwise, returns an unlocked object lock.
 *
 * @returns An object lock.
 */
ObjectLock Namespace::LockIfRequired()
{
	if (Frozen()) {
		return ObjectLock(this, std::defer_lock);
	}
	return ObjectLock(this);
}

std::shared_lock<std::shared_timed_mutex> Namespace::ReadLockUnlessFrozen() const
{
	if (m_Frozen.load(std::memory_order_relaxed)) {
		return std::shared_lock<std::shared_timed_mutex>();
	} else {
		return std::shared_lock<std::shared_timed_mutex>(m_DataMutex);
	}
}

Value Namespace::GetFieldByName(const String& field, bool, const DebugInfo& debugInfo) const
{
	auto lock (ReadLockUnlessFrozen());

	auto nsVal = m_Data.find(field);

	if (nsVal != m_Data.end())
		return nsVal->second.Val;
	else
		return GetPrototypeField(const_cast<Namespace *>(this), field, false, debugInfo); /* Ignore indexer not found errors similar to the Dictionary class. */
}

void Namespace::SetFieldByName(const String& field, const Value& value, const DebugInfo& debugInfo)
{
	Set(field, value, false, debugInfo);
}

bool Namespace::HasOwnField(const String& field) const
{
	return Contains(field);
}

bool Namespace::GetOwnField(const String& field, Value *result) const
{
	return Get(field, result);
}

Namespace::Iterator Namespace::Begin()
{
	ASSERT(Frozen() || OwnsLock());

	return m_Data.begin();
}

Namespace::Iterator Namespace::End()
{
	ASSERT(Frozen() || OwnsLock());

	return m_Data.end();
}

Namespace::Iterator icinga::begin(const Namespace::Ptr& x)
{
	return x->Begin();
}

Namespace::Iterator icinga::end(const Namespace::Ptr& x)
{
	return x->End();
}
