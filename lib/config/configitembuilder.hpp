/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "config/expression.hpp"
#include "config/configitem.hpp"
#include "base/debuginfo.hpp"
#include "base/object.hpp"

namespace icinga
{

/**
 * Config item builder. Used to dynamically build configuration objects
 * at runtime.
 *
 * @ingroup config
 */
class ConfigItemBuilder final
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigItemBuilder);

	ConfigItemBuilder() = default;
	explicit ConfigItemBuilder(const DebugInfo& debugInfo);

	void SetType(const Type::Ptr& type);
	void SetName(const String& name);
	void SetAbstract(bool abstract);
	void SetScope(const Dictionary::Ptr& scope);
	void SetZone(const String& zone);
	void SetPackage(const String& package);
	void SetDefaultTemplate(bool defaultTmpl);
	void SetIgnoreOnError(bool ignoreOnError);

	void AddExpression(Expression *expr);
	void SetFilter(const Expression::Ptr& filter);

	ConfigItem::Ptr Compile();

private:
	Type::Ptr m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	bool m_Abstract{false}; /**< Whether the item is abstract. */
	std::vector<std::unique_ptr<Expression> > m_Expressions; /**< Expressions for this item. */
	Expression::Ptr m_Filter; /**< Filter expression. */
	DebugInfo m_DebugInfo; /**< Debug information. */
	Dictionary::Ptr m_Scope; /**< variable scope. */
	String m_Zone; /**< The zone. */
	String m_Package; /**< The package name. */
	bool m_DefaultTmpl{false};
	bool m_IgnoreOnError{false}; /**< Whether the object should be ignored when an error occurs in one of the expressions. */
};

}

#endif /* CONFIGITEMBUILDER */
