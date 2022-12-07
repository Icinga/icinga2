/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/namespace.hpp"
#include "base/objectlock.hpp"
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
	ObjectLock olock(LockUnlessFrozen());

	Value value;
	if (!GetOwnField(field, &value))
		BOOST_THROW_EXCEPTION(ScriptError("Namespace does not contain field '" + field + "'"));
	return value;
}

bool Namespace::Get(const String& field, Value *value) const
{
	ObjectLock olock(LockUnlessFrozen());

	auto nsVal = GetAttribute(field);

	if (!nsVal)
		return false;

	*value =  nsVal->Get(DebugInfo());
	return true;
}

void Namespace::Set(const String& field, const Value& value)
{
	ObjectLock olock(this);

	return SetFieldByName(field, value, false, DebugInfo());
}

/**
 * Returns the number of elements in the namespace.
 *
 * @returns Number of elements.
 */
size_t Namespace::GetLength() const
{
	ObjectLock olock(LockUnlessFrozen());

	return m_Data.size();
}

bool Namespace::Contains(const String& field) const
{
	ObjectLock olock(LockUnlessFrozen());

	return HasOwnField(field);
}

void Namespace::Remove(const String& field)
{
	ObjectLock olock(this);

	if (m_Frozen) {
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified."));
	}

	auto attr = GetAttribute(field);

	if (dynamic_pointer_cast<ConstEmbeddedNamespaceValue>(attr)) {
		BOOST_THROW_EXCEPTION(ScriptError("Constants must not be removed."));
	}

	RemoveAttribute(field);
}

/**
 * Freeze the namespace, preventing further updates.
 *
 * This only prevents inserting, replacing or deleting values from the namespace. This operation has no effect on
 * objects referenced by the values, these remain mutable if they were before.
 */
void Namespace::Freeze() {
	ObjectLock olock(this);

	m_Frozen = true;
}

ObjectLock Namespace::LockUnlessFrozen() const
{
	if (m_Frozen.load(std::memory_order_relaxed)) {
		return ObjectLock(nullptr);
	} else {
		return ObjectLock(this);
	}
}

void Namespace::RemoveAttribute(const String& field)
{
	ObjectLock olock(this);

	Namespace::Iterator it;
	it = m_Data.find(field);

	if (it == m_Data.end())
		return;

	m_Data.erase(it);
}

NamespaceValue::Ptr Namespace::GetAttribute(const String& key) const
{
	ObjectLock olock(LockUnlessFrozen());

	auto it = m_Data.find(key);

	if (it == m_Data.end())
		return nullptr;

	return it->second;
}

void Namespace::SetAttribute(const String& key, const NamespaceValue::Ptr& nsVal)
{
	ObjectLock olock(this);

	m_Data[key] = nsVal;
}

Value Namespace::GetFieldByName(const String& field, bool, const DebugInfo& debugInfo) const
{
	ObjectLock olock(LockUnlessFrozen());

	auto nsVal = GetAttribute(field);

	if (nsVal)
		return nsVal->Get(debugInfo);
	else
		return GetPrototypeField(const_cast<Namespace *>(this), field, false, debugInfo); /* Ignore indexer not found errors similar to the Dictionary class. */
}

void Namespace::SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	// The override frozen parameter is mandated by the interface but ignored here. If the namespace is frozen, this
	// disables locking for read operations, so it must not be modified again to ensure the consistency of the internal
	// data structures.
	(void) overrideFrozen;

	ObjectLock olock(this);

	auto nsVal = GetAttribute(field);

	if (!nsVal) {
		if (m_Frozen) {
			BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified.", debugInfo));
		}

		if (m_ConstValues) {
			SetAttribute(field, new ConstEmbeddedNamespaceValue(value));
		} else {
			SetAttribute(field, new EmbeddedNamespaceValue(value));
		}
	} else {
		nsVal->Set(value, debugInfo);
	}
}

bool Namespace::HasOwnField(const String& field) const
{
	ObjectLock olock(LockUnlessFrozen());

	return GetAttribute(field) != nullptr;
}

bool Namespace::GetOwnField(const String& field, Value *result) const
{
	ObjectLock olock(LockUnlessFrozen());

	auto nsVal = GetAttribute(field);

	if (!nsVal)
		return false;

	*result = nsVal->Get(DebugInfo());
	return true;
}

EmbeddedNamespaceValue::EmbeddedNamespaceValue(const Value& value)
	: m_Value(value)
{ }

Value EmbeddedNamespaceValue::Get(const DebugInfo& debugInfo) const
{
	return m_Value;
}

void EmbeddedNamespaceValue::Set(const Value& value, const DebugInfo&)
{
	m_Value = value;
}

void ConstEmbeddedNamespaceValue::Set(const Value& value, const DebugInfo& debugInfo)
{
	BOOST_THROW_EXCEPTION(ScriptError("Constant must not be modified.", debugInfo));
}

Namespace::Iterator Namespace::Begin()
{
	ASSERT(OwnsLock());

	return m_Data.begin();
}

Namespace::Iterator Namespace::End()
{
	ASSERT(OwnsLock());

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

