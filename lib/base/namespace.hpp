/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/shared-object.hpp"
#include "base/value.hpp"
#include "base/debuginfo.hpp"
#include <map>
#include <vector>
#include <memory>

namespace icinga
{

struct NamespaceValue : public SharedObject
{
	DECLARE_PTR_TYPEDEFS(NamespaceValue);

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

	typedef std::map<String, NamespaceValue::Ptr>::iterator Iterator;

	typedef std::map<String, NamespaceValue::Ptr>::value_type Pair;

	Namespace(NamespaceBehavior *behavior = new NamespaceBehavior);

	Value Get(const String& field) const;
	bool Get(const String& field, Value *value) const;
	void Set(const String& field, const Value& value, bool overrideFrozen = false);
	bool Contains(const String& field) const;
	void Remove(const String& field, bool overrideFrozen = false);

	NamespaceValue::Ptr GetAttribute(const String& field) const;
	void SetAttribute(const String& field, const NamespaceValue::Ptr& nsVal);
	void RemoveAttribute(const String& field);

	Iterator Begin();
	Iterator End();

	size_t GetLength() const;

	Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const override;
	void SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;
	bool HasOwnField(const String& field) const override;
	bool GetOwnField(const String& field, Value *result) const override;

	static Object::Ptr GetPrototype();

private:
	std::map<String, NamespaceValue::Ptr> m_Data;
	std::unique_ptr<NamespaceBehavior> m_Behavior;
};

Namespace::Iterator begin(const Namespace::Ptr& x);
Namespace::Iterator end(const Namespace::Ptr& x);

}

extern template class std::map<icinga::String, std::shared_ptr<icinga::NamespaceValue> >;

#endif /* NAMESPACE_H */
