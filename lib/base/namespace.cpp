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

Namespace::Namespace(NamespaceBehavior *behavior)
	: m_Behavior(std::unique_ptr<NamespaceBehavior>(behavior))
{ }

Value Namespace::Get(const String& field) const
{
	ObjectLock olock(this);

	Value value;
	if (!GetOwnField(field, &value))
		BOOST_THROW_EXCEPTION(ScriptError("Namespace does not contain field '" + field + "'"));
	return value;
}

bool Namespace::Get(const String& field, Value *value) const
{
	ObjectLock olock(this);

	auto nsVal = GetAttribute(field);

	if (!nsVal)
		return false;

	*value =  nsVal->Get(DebugInfo());
	return true;
}

void Namespace::Set(const String& field, const Value& value, bool overrideFrozen)
{
	ObjectLock olock(this);

	return SetFieldByName(field, value, overrideFrozen, DebugInfo());
}

bool Namespace::Contains(const String& field) const
{
	ObjectLock olock(this);

	return HasOwnField(field);
}

void Namespace::Remove(const String& field, bool overrideFrozen)
{
	ObjectLock olock(this);

	m_Behavior->Remove(this, field, overrideFrozen);
}

Namespace::Ptr Namespace::ShallowClone() const
{
	Ptr copy (new Namespace());
	ObjectLock lock (this);

	copy->m_Data = m_Data;
	copy->m_Behavior = m_Behavior->Clone();

	for (auto& kv : copy->m_Data) {
		kv.second = kv.second->ShallowClone();
	}

	return copy;
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
	ObjectLock olock(this);

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
	ObjectLock olock(this);

	auto nsVal = GetAttribute(field);

	if (nsVal)
		return nsVal->Get(debugInfo);
	else
		return GetPrototypeField(const_cast<Namespace *>(this), field, false, debugInfo); /* Ignore indexer not found errors similar to the Dictionary class. */
}

void Namespace::SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	ObjectLock olock(this);

	auto nsVal = GetAttribute(field);

	if (!nsVal)
		m_Behavior->Register(this, field, value, overrideFrozen, debugInfo);
	else
		nsVal->Set(value, overrideFrozen, debugInfo);
}

bool Namespace::HasOwnField(const String& field) const
{
	ObjectLock olock(this);

	return GetAttribute(field) != nullptr;
}

bool Namespace::GetOwnField(const String& field, Value *result) const
{
	ObjectLock olock(this);

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

void EmbeddedNamespaceValue::Set(const Value& value, bool, const DebugInfo&)
{
	m_Value = value;
}

NamespaceValue::Ptr EmbeddedNamespaceValue::ShallowClone() const
{
	return new EmbeddedNamespaceValue(*this);
}

void ConstEmbeddedNamespaceValue::Set(const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	if (!overrideFrozen)
		BOOST_THROW_EXCEPTION(ScriptError("Constant must not be modified.", debugInfo));

	EmbeddedNamespaceValue::Set(value, overrideFrozen, debugInfo);
}

NamespaceValue::Ptr ConstEmbeddedNamespaceValue::ShallowClone() const
{
	return new ConstEmbeddedNamespaceValue(*this);
}

void NamespaceBehavior::Register(const Namespace::Ptr& ns, const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) const
{
	ns->SetAttribute(field, new EmbeddedNamespaceValue(value));
}

void NamespaceBehavior::Remove(const Namespace::Ptr& ns, const String& field, bool overrideFrozen)
{
	if (!overrideFrozen) {
		auto attr = ns->GetAttribute(field);

		if (dynamic_pointer_cast<ConstEmbeddedNamespaceValue>(attr))
			BOOST_THROW_EXCEPTION(ScriptError("Constants must not be removed."));
	}

	ns->RemoveAttribute(field);
}

std::unique_ptr<NamespaceBehavior> NamespaceBehavior::Clone() const
{
	return std::unique_ptr<NamespaceBehavior>(new NamespaceBehavior(*this));
}

void ConstNamespaceBehavior::Register(const Namespace::Ptr& ns, const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) const
{
	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified.", debugInfo));

	ns->SetAttribute(field, new ConstEmbeddedNamespaceValue(value));
}

void ConstNamespaceBehavior::Remove(const Namespace::Ptr& ns, const String& field, bool overrideFrozen)
{
	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified."));

	NamespaceBehavior::Remove(ns, field, overrideFrozen);
}

std::unique_ptr<NamespaceBehavior> ConstNamespaceBehavior::Clone() const
{
	return std::unique_ptr<NamespaceBehavior>(new ConstNamespaceBehavior(*this));
}

void ConstNamespaceBehavior::Freeze()
{
	m_Frozen = true;
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

