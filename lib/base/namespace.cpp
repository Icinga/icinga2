/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/namespace.hpp"
#include "base/objectlock.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/debuginfo.hpp"
#include "base/exception.hpp"
#include <sstream>
#include <utility>

using namespace icinga;

template class icinga::HybridMap<String, NamespaceValue>;

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
	NamespaceValue nv;

	if (!m_Data.Get(field, nv)) {
		return false;
	}

	*value = std::move(nv.Val);
	return true;
}

void Namespace::Set(const String& field, const Value& value, bool isConst, const DebugInfo& debugInfo)
{
	ObjectLock olock(this);

	if (m_Frozen) {
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified.", debugInfo));
	}

	std::unique_lock<decltype(m_DataMutex)> dlock (m_DataMutex);

	auto pos (m_Data.Find(field));

	if (pos == m_Data.NPos()) {
		m_Data.Set(field, NamespaceValue{value, isConst || m_ConstValues});
	} else {
		if (pos->second.Const) {
			BOOST_THROW_EXCEPTION(ScriptError("Constant must not be modified.", debugInfo));
		}

		*const_cast<Value*>(&pos->second.Val) = value;
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

	return m_Data.GetLength();
}

bool Namespace::Contains(const String& field) const
{
	auto lock (ReadLockUnlessFrozen());

	return m_Data.Contains(field);
}

void Namespace::Remove(const String& field)
{
	ObjectLock olock(this);

	if (m_Frozen) {
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified."));
	}

	std::unique_lock<decltype(m_DataMutex)> dlock (m_DataMutex);

	auto pos (m_Data.Find(field));

	if (pos == m_Data.NPos()) {
		return;
	}

	if (pos->second.Const) {
		BOOST_THROW_EXCEPTION(ScriptError("Constants must not be removed."));
	}

	m_Data.Remove(pos);
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
	NamespaceValue nv;
	auto lock (ReadLockUnlessFrozen());

	if (m_Data.Get(field, nv)) {
		return std::move(nv.Val);
	} else {
		return GetPrototypeField(const_cast<Namespace *>(this), field, false, debugInfo); /* Ignore indexer not found errors similar to the Dictionary class. */
	}
}

void Namespace::SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	// The override frozen parameter is mandated by the interface but ignored here. If the namespace is frozen, this
	// disables locking for read operations, so it must not be modified again to ensure the consistency of the internal
	// data structures.
	(void) overrideFrozen;

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
