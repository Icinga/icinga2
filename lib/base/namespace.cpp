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
	Value value;
	if (!Get(field, &value))
		BOOST_THROW_EXCEPTION(ScriptError("Namespace does not contain field '" + field + "'"));
	return value;
}

bool Namespace::Get(const String& field, Value *value) const
{
	std::shared_lock<decltype(m_DataMutex)> lock(m_DataMutex);

	auto nsVal = m_Data.find(field);

	if (nsVal == m_Data.end()) {
		return false;
	}

	*value = nsVal->second.Val;
	return true;
}

void Namespace::Set(const String& field, const Value& value, bool isConst, bool overrideFrozen, const DebugInfo& debugInfo)
{
	ObjectLock olock(this);
	std::unique_lock<decltype(m_DataMutex)> dlock (m_DataMutex);

	auto nsVal = m_Data.find(field);

	if (nsVal == m_Data.end()) {
		if (m_Frozen && !overrideFrozen) {
			BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified.", debugInfo));
		}

		m_Data[field] = NamespaceValue{value, isConst};
	} else {
		if (nsVal->second.Const && !overrideFrozen) {
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
	std::shared_lock<decltype(m_DataMutex)> lock(m_DataMutex);

	return m_Data.size();
}

bool Namespace::Contains(const String& field) const
{
	std::shared_lock<decltype(m_DataMutex)> lock(m_DataMutex);

	return m_Data.find(field) != m_Data.end();
}

void Namespace::Remove(const String& field, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen) {
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified."));
	}

	std::unique_lock<decltype(m_DataMutex)> dlock (m_DataMutex);

	if (!overrideFrozen) {
		auto attr = m_Data.find(field);

		if (attr != m_Data.end() && attr->second.Const) {
			BOOST_THROW_EXCEPTION(ScriptError("Constants must not be removed."));
		}
	}

	auto it = m_Data.find(field);

	if (it == m_Data.end())
		return;

	m_Data.erase(it);
}

/**
 * Freeze the namespace, preventing further updates unless overrideFrozen is set.
 *
 * This only prevents inserting, replacing or deleting values from the namespace. This operation has no effect on
 * objects referenced by the values, these remain mutable if they were before.
 */
void Namespace::Freeze() {
	ObjectLock olock(this);

	m_Frozen = true;
}

Value Namespace::GetFieldByName(const String& field, bool, const DebugInfo& debugInfo) const
{
	std::shared_lock<decltype(m_DataMutex)> lock(m_DataMutex);

	auto nsVal = m_Data.find(field);

	if (nsVal != m_Data.end())
		return nsVal->second.Val;
	else
		return GetPrototypeField(const_cast<Namespace *>(this), field, false, debugInfo); /* Ignore indexer not found errors similar to the Dictionary class. */
}

void Namespace::SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	Set(field, value, false, overrideFrozen, debugInfo);
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

