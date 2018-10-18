/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
	Value value;
	if (!GetOwnField(field, &value))
		BOOST_THROW_EXCEPTION(ScriptError("Namespace does not contain field '" + field + "'"));
	return value;
}

bool Namespace::Get(const String& field, Value *value) const
{
	auto nsVal = GetAttribute(field);

	if (!nsVal)
		return false;

	*value =  nsVal->Get(DebugInfo());
	return true;
}

void Namespace::Set(const String& field, const Value& value, bool overrideFrozen)
{
	return SetFieldByName(field, value, overrideFrozen, DebugInfo());
}

bool Namespace::Contains(const String& field) const
{
	return HasOwnField(field);
}

void Namespace::Remove(const String& field, bool overrideFrozen)
{
	m_Behavior->Remove(this, field, overrideFrozen);
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

std::shared_ptr<NamespaceValue> Namespace::GetAttribute(const String& key) const
{
	ObjectLock olock(this);

	auto it = m_Data.find(key);

	if (it == m_Data.end())
		return nullptr;

	return it->second;
}

void Namespace::SetAttribute(const String& key, const std::shared_ptr<NamespaceValue>& nsVal)
{
	ObjectLock olock(this);

	m_Data[key] = nsVal;
}

Value Namespace::GetFieldByName(const String& field, bool, const DebugInfo& debugInfo) const
{
	auto nsVal = GetAttribute(field);

	if (nsVal)
		return nsVal->Get(debugInfo);
	else
		return GetPrototypeField(const_cast<Namespace *>(this), field, false, debugInfo); /* Ignore indexer not found errors similar to the Dictionary class. */
}

void Namespace::SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	auto nsVal = GetAttribute(field);

	if (!nsVal)
		m_Behavior->Register(this, field, value, overrideFrozen, debugInfo);
	else
		nsVal->Set(value, overrideFrozen, debugInfo);
}

bool Namespace::HasOwnField(const String& field) const
{
	return GetAttribute(field) != nullptr;
}

bool Namespace::GetOwnField(const String& field, Value *result) const
{
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

void ConstEmbeddedNamespaceValue::Set(const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	if (!overrideFrozen)
		BOOST_THROW_EXCEPTION(ScriptError("Constant must not be modified.", debugInfo));

	EmbeddedNamespaceValue::Set(value, overrideFrozen, debugInfo);
}

void NamespaceBehavior::Register(const Namespace::Ptr& ns, const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) const
{
	ns->SetAttribute(field, std::make_shared<EmbeddedNamespaceValue>(value));
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

void ConstNamespaceBehavior::Register(const Namespace::Ptr& ns, const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) const
{
	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified.", debugInfo));

	ns->SetAttribute(field, std::make_shared<ConstEmbeddedNamespaceValue>(value));
}

void ConstNamespaceBehavior::Remove(const Namespace::Ptr& ns, const String& field, bool overrideFrozen)
{
	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(ScriptError("Namespace is read-only and must not be modified."));

	NamespaceBehavior::Remove(ns, field, overrideFrozen);
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

