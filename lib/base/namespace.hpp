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

struct NamespaceValue
{
	Value Val;
	bool Const;
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

	typedef std::map<String, NamespaceValue>::iterator Iterator;

	typedef std::map<String, NamespaceValue>::value_type Pair;

	explicit Namespace(bool constValues = false);

	Value Get(const String& field) const;
	bool Get(const String& field, Value *value) const;
	void Set(const String& field, const Value& value, bool isConst = false, bool overrideFrozen = false, const DebugInfo& debugInfo = DebugInfo());
	bool Contains(const String& field) const;
	void Remove(const String& field, bool overrideFrozen = false);
	void Freeze();

	Iterator Begin();
	Iterator End();

	size_t GetLength() const;

	Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const override;
	void SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;
	bool HasOwnField(const String& field) const override;
	bool GetOwnField(const String& field, Value *result) const override;

	static Object::Ptr GetPrototype();

private:
	std::map<String, NamespaceValue> m_Data;
	bool m_ConstValues;
	bool m_Frozen;
};

Namespace::Iterator begin(const Namespace::Ptr& x);
Namespace::Iterator end(const Namespace::Ptr& x);

}

extern template class std::map<icinga::String, std::shared_ptr<icinga::NamespaceValue> >;

#endif /* NAMESPACE_H */
