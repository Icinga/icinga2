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

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/value.hpp"
#include "base/debuginfo.hpp"
#include <map>
#include <vector>
#include <memory>

namespace icinga
{

struct NamespaceValue
{
	virtual Value Get(const DebugInfo& debugInfo = DebugInfo()) const = 0;
	virtual void Set(const Value& value, bool overrideFrozen, const DebugInfo& debugInfo = DebugInfo()) = 0;
};

struct EmbeddedNamespaceValue : public NamespaceValue
{
	EmbeddedNamespaceValue(const Value& value);

	Value Get(const DebugInfo& debugInfo) const override;
	void Set(const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;

private:
	Value m_Value;
};

struct ConstEmbeddedNamespaceValue : public EmbeddedNamespaceValue
{
	using EmbeddedNamespaceValue::EmbeddedNamespaceValue;

	void Set(const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;
};

class Namespace;

struct NamespaceBehavior
{
	virtual void Register(const boost::intrusive_ptr<Namespace>& ns, const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) const;
	virtual void Remove(const boost::intrusive_ptr<Namespace>& ns, const String& field, bool overrideFrozen);
};

struct ConstNamespaceBehavior : public NamespaceBehavior
{
	void Register(const boost::intrusive_ptr<Namespace>& ns, const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) const override;
	void Remove(const boost::intrusive_ptr<Namespace>& ns, const String& field, bool overrideFrozen) override;
	void Freeze();

private:
	bool m_Frozen;
};

/**
 * A namespace.
 *
 * @ingroup base
 */
class Namespace final : public Object
{
public:
	DECLARE_OBJECT(Namespace);

	typedef std::map<String, std::shared_ptr<NamespaceValue> >::iterator Iterator;

	typedef std::map<String, std::shared_ptr<NamespaceValue> >::value_type Pair;

	Namespace(NamespaceBehavior *behavior = new NamespaceBehavior);

	Value Get(const String& field) const;
	bool Get(const String& field, Value *value) const;
	void Set(const String& field, const Value& value, bool overrideFrozen = false);
	bool Contains(const String& field) const;
	void Remove(const String& field, bool overrideFrozen = false);

	std::shared_ptr<NamespaceValue> GetAttribute(const String& field) const;
	void SetAttribute(const String& field, const std::shared_ptr<NamespaceValue>& nsVal);
	void RemoveAttribute(const String& field);

	Iterator Begin();
	Iterator End();

	Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const override;
	void SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;
	bool HasOwnField(const String& field) const override;
	bool GetOwnField(const String& field, Value *result) const override;

	static Object::Ptr GetPrototype();

private:
	std::map<String, std::shared_ptr<NamespaceValue> > m_Data;
	std::unique_ptr<NamespaceBehavior> m_Behavior;
};

Namespace::Iterator begin(const Namespace::Ptr& x);
Namespace::Iterator end(const Namespace::Ptr& x);

}

extern template class std::map<icinga::String, std::shared_ptr<icinga::NamespaceValue> >;

#endif /* NAMESPACE_H */
