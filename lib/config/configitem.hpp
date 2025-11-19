/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGITEM_H
#define CONFIGITEM_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "config/activationcontext.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"
#include <functional>

namespace icinga
{


/**
 * A configuration item. Non-abstract configuration items can be used to
 * create configuration objects at runtime.
 *
 * @ingroup config
 */
class ConfigItem final : public Object {
public:
	DECLARE_PTR_TYPEDEFS(ConfigItem);

	ConfigItem(Type::Ptr type, String name, bool abstract,
		Expression::Ptr exprl,
		Expression::Ptr filter,
		bool defaultTmpl, bool ignoreOnError, DebugInfo debuginfo,
		Dictionary::Ptr scope, String zone,
		String package);

	Type::Ptr GetType() const;
	String GetName() const;
	bool IsAbstract() const;
	bool IsDefaultTemplate() const;
	bool IsIgnoreOnError() const;

	std::vector<ConfigItem::Ptr> GetParents() const;

	Expression::Ptr GetExpression() const;
	Expression::Ptr GetFilter() const;

	void Register();
	void Unregister();
	ConfigObject::Ptr Commit(bool discard = true);

	DebugInfo GetDebugInfo() const;
	Dictionary::Ptr GetScope() const;

	ConfigObject::Ptr GetObject() const;

	static ConfigItem::Ptr GetByTypeAndName(const Type::Ptr& type,
		const String& name);

	static bool CommitItems(const ActivationContext::Ptr& context, WorkQueue& upq, std::vector<ConfigItem::Ptr>& newItems, bool silent = false);
	static bool ActivateItems(const std::vector<ConfigItem::Ptr>& newItems, bool runtimeCreated = false,
		bool mainConfigActivation = false, bool withModAttrs = false, const Value& cookie = Empty);

	static bool RunWithActivationContext(const Function::Ptr& function);

	static std::vector<ConfigItem::Ptr> GetItems(const Type::Ptr& type);
	static std::vector<ConfigItem::Ptr> GetDefaultTemplates(const Type::Ptr& type);

	static void RemoveIgnoredItems(const String& allowedConfigPath);

	static thread_local std::function<void(ConfigItem*)> m_OverrideRegistry;

private:
	Type::Ptr m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	bool m_Abstract; /**< Whether this is a template. */

	Expression::Ptr m_Expression;
	Expression::Ptr m_Filter;
	bool m_DefaultTmpl;
	bool m_IgnoreOnError;
	DebugInfo m_DebugInfo; /**< Debug information. */
	Dictionary::Ptr m_Scope; /**< variable scope. */
	String m_Zone; /**< The zone. */
	String m_Package;
	ActivationContext::Ptr m_ActivationContext;

	ConfigObject::Ptr m_Object;

	static std::mutex m_Mutex;

	typedef std::map<String, ConfigItem::Ptr> ItemMap;
	typedef std::map<Type::Ptr, ItemMap> TypeMap;
	static TypeMap m_Items; /**< All registered configuration items. */
	static TypeMap m_DefaultTemplates;

	typedef std::vector<ConfigItem::Ptr> ItemList;
	static ItemList m_UnnamedItems;

	typedef std::vector<String> IgnoredItemList;
	static IgnoredItemList m_IgnoredItems;

	static ConfigItem::Ptr GetObjectUnlocked(const String& type,
		const String& name);

	static bool CommitNewItems(const ActivationContext::Ptr& context, WorkQueue& upq, std::vector<ConfigItem::Ptr>& newItems);
};

/**
 * Helper class for exposing config items being committed to the ConfigType::BeforeOnAllConfigLoaded callback.
 *
 * This class wraps a reference to an internal data structure used in ConfigItem::CommitNewItems() and provides
 * functions useful for the callbacks without exposing the internals of CommitNewItems().
 */
class ConfigItems
{
	explicit ConfigItems(std::vector<std::pair<ConfigItem::Ptr, bool>>& items) : m_Items(items) {}

	std::vector<std::pair<ConfigItem::Ptr, bool>>& m_Items;

	friend ConfigItem;

public:
	/**
	 * ForEachObject<T>(f) calls f(t) for each object T::Ptr t in vector of underlying config items.
	 *
	 * @tparam T ConfigObject type to iterate over
	 * @tparam F Callback functor type (usually automatically deduced from func)
	 * @param func Functor accepting T::Ptr as an argument to be called for each object
	 */
	template<typename T, typename F>
	void ForEachObject(F func) const
	{
		for (const auto& item : m_Items) {
			if (typename T::Ptr obj = dynamic_pointer_cast<T>(item.first->GetObject()); obj) {
				func(std::move(obj));
			}
		}
	}
};

}

#endif /* CONFIGITEM_H */
