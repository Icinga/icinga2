/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "config/configitem.hpp"
#include "config/configcompilercontext.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "config/configcompiler.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/debug.hpp"
#include "base/workqueue.hpp"
#include "base/exception.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include "base/exception.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/join.hpp>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <random>

using namespace icinga;

std::mutex ConfigItem::m_Mutex;
ConfigItem::TypeMap ConfigItem::m_Items;
ConfigItem::TypeMap ConfigItem::m_DefaultTemplates;
ConfigItem::ItemList ConfigItem::m_UnnamedItems;
ConfigItem::IgnoredItemList ConfigItem::m_IgnoredItems;

REGISTER_FUNCTION(Internal, run_with_activation_context, &ConfigItem::RunWithActivationContext, "func");

/**
 * Constructor for the ConfigItem class.
 *
 * @param type The object type.
 * @param name The name of the item.
 * @param unit The unit of the item.
 * @param abstract Whether the item is a template.
 * @param exprl Expression list for the item.
 * @param debuginfo Debug information.
 */
ConfigItem::ConfigItem(Type::Ptr type, String name,
	bool abstract, Expression::Ptr exprl,
	Expression::Ptr filter, bool defaultTmpl, bool ignoreOnError,
	DebugInfo debuginfo, Dictionary::Ptr scope,
	String zone, String package)
	: m_Type(std::move(type)), m_Name(std::move(name)), m_Abstract(abstract),
	m_Expression(std::move(exprl)), m_Filter(std::move(filter)),
	m_DefaultTmpl(defaultTmpl), m_IgnoreOnError(ignoreOnError),
	m_DebugInfo(std::move(debuginfo)), m_Scope(std::move(scope)), m_Zone(std::move(zone)),
	m_Package(std::move(package))
{
}

/**
 * Retrieves the type of the configuration item.
 *
 * @returns The type.
 */
Type::Ptr ConfigItem::GetType() const
{
	return m_Type;
}

/**
 * Retrieves the name of the configuration item.
 *
 * @returns The name.
 */
String ConfigItem::GetName() const
{
	return m_Name;
}

/**
 * Checks whether the item is abstract.
 *
 * @returns true if the item is abstract, false otherwise.
 */
bool ConfigItem::IsAbstract() const
{
	return m_Abstract;
}

bool ConfigItem::IsDefaultTemplate() const
{
	return m_DefaultTmpl;
}

bool ConfigItem::IsIgnoreOnError() const
{
	return m_IgnoreOnError;
}

/**
 * Retrieves the debug information for the configuration item.
 *
 * @returns The debug information.
 */
DebugInfo ConfigItem::GetDebugInfo() const
{
	return m_DebugInfo;
}

Dictionary::Ptr ConfigItem::GetScope() const
{
	return m_Scope;
}

ConfigObject::Ptr ConfigItem::GetObject() const
{
	return m_Object;
}

/**
 * Retrieves the expression list for the configuration item.
 *
 * @returns The expression list.
 */
Expression::Ptr ConfigItem::GetExpression() const
{
	return m_Expression;
}

/**
* Retrieves the object filter for the configuration item.
*
* @returns The filter expression.
*/
Expression::Ptr ConfigItem::GetFilter() const
{
	return m_Filter;
}

class DefaultValidationUtils final : public ValidationUtils
{
public:
	bool ValidateName(const String& type, const String& name) const override
	{
		ConfigItem::Ptr item = ConfigItem::GetByTypeAndName(Type::GetByName(type), name);

		if (!item || (item && item->IsAbstract()))
			return false;

		return true;
	}
};

/**
 * Commits the configuration item by creating a ConfigObject
 * object.
 *
 * @returns The ConfigObject that was created/updated.
 */
ConfigObject::Ptr ConfigItem::Commit(bool discard)
{
	Type::Ptr type = GetType();

#ifdef I2_DEBUG
	Log(LogDebug, "ConfigItem")
		<< "Commit called for ConfigItem Type=" << type->GetName() << ", Name=" << GetName();
#endif /* I2_DEBUG */

	/* Make sure the type is valid. */
	if (!type || !ConfigObject::TypeInstance->IsAssignableFrom(type))
		BOOST_THROW_EXCEPTION(ScriptError("Type '" + type->GetName() + "' does not exist.", m_DebugInfo));

	if (IsAbstract())
		return nullptr;

	ConfigObject::Ptr dobj = static_pointer_cast<ConfigObject>(type->Instantiate(std::vector<Value>()));

	dobj->SetDebugInfo(m_DebugInfo);
	dobj->SetZoneName(m_Zone);
	dobj->SetPackage(m_Package);
	dobj->SetName(m_Name);

	DebugHint debugHints;

	ScriptFrame frame(true, dobj);
	if (m_Scope)
		m_Scope->CopyTo(frame.Locals);
	try {
		m_Expression->Evaluate(frame, &debugHints);
	} catch (const std::exception& ex) {
		if (m_IgnoreOnError) {
			Log(LogNotice, "ConfigObject")
				<< "Ignoring config object '" << m_Name << "' of type '" << type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_IgnoredItems.push_back(m_DebugInfo.Path);
			}

			return nullptr;
		}

		throw;
	}

	if (discard)
		m_Expression.reset();

	String item_name;
	String short_name = dobj->GetShortName();

	if (!short_name.IsEmpty()) {
		item_name = short_name;
		dobj->SetName(short_name);
	} else
		item_name = m_Name;

	String name = item_name;

	auto *nc = dynamic_cast<NameComposer *>(type.get());

	if (nc) {
		if (name.IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Object name must not be empty.", m_DebugInfo));

		name = nc->MakeName(name, dobj);

		if (name.IsEmpty())
			BOOST_THROW_EXCEPTION(std::runtime_error("Could not determine name for object"));
	}

	if (name != item_name)
		dobj->SetShortName(item_name);

	dobj->SetName(name);

	Dictionary::Ptr dhint = debugHints.ToDictionary();

	try {
		DefaultValidationUtils utils;
		dobj->Validate(FAConfig, utils);
	} catch (ValidationError& ex) {
		if (m_IgnoreOnError) {
			Log(LogNotice, "ConfigObject")
				<< "Ignoring config object '" << m_Name << "' of type '" << type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_IgnoredItems.push_back(m_DebugInfo.Path);
			}

			return nullptr;
		}

		ex.SetDebugHint(dhint);
		throw;
	}

	try {
		dobj->OnConfigLoaded();
	} catch (const std::exception& ex) {
		if (m_IgnoreOnError) {
			Log(LogNotice, "ConfigObject")
				<< "Ignoring config object '" << m_Name << "' of type '" << m_Type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_IgnoredItems.push_back(m_DebugInfo.Path);
			}

			return nullptr;
		}

		throw;
	}

	Value serializedObject;

	try {
		serializedObject = Serialize(dobj, FAConfig);
	} catch (const CircularReferenceError& ex) {
		BOOST_THROW_EXCEPTION(ValidationError(dobj, ex.GetPath(), "Circular references are not allowed"));
	}

	Dictionary::Ptr persistentItem = new Dictionary({
		{ "type", type->GetName() },
		{ "name", GetName() },
		{ "properties", Serialize(dobj, FAConfig) },
		{ "debug_hints", dhint },
		{ "debug_info", new Array({
			m_DebugInfo.Path,
			m_DebugInfo.FirstLine,
			m_DebugInfo.FirstColumn,
			m_DebugInfo.LastLine,
			m_DebugInfo.LastColumn,
		}) }
	});

	dhint.reset();

	ConfigCompilerContext::GetInstance()->WriteObject(persistentItem);
	persistentItem.reset();

	dobj->Register();

	m_Object = dobj;

	return dobj;
}

/**
 * Registers the configuration item.
 */
void ConfigItem::Register()
{
	m_ActivationContext = ActivationContext::GetCurrentContext();

	std::unique_lock<std::mutex> lock(m_Mutex);

	/* If this is a non-abstract object with a composite name
	 * we register it in m_UnnamedItems instead of m_Items. */
	if (!m_Abstract && dynamic_cast<NameComposer *>(m_Type.get()))
		m_UnnamedItems.emplace_back(this);
	else {
		auto& items = m_Items[m_Type];

		auto it = items.find(m_Name);

		if (it != items.end()) {
			std::ostringstream msgbuf;
			msgbuf << "A configuration item of type '" << m_Type->GetName()
					<< "' and name '" << GetName() << "' already exists ("
					<< it->second->GetDebugInfo() << "), new declaration: " << GetDebugInfo();
			BOOST_THROW_EXCEPTION(ScriptError(msgbuf.str()));
		}

		m_Items[m_Type][m_Name] = this;

		if (m_DefaultTmpl)
			m_DefaultTemplates[m_Type][m_Name] = this;
	}
}

/**
 * Unregisters the configuration item.
 */
void ConfigItem::Unregister()
{
	if (m_Object) {
		m_Object->Unregister();
		m_Object.reset();
	}

	std::unique_lock<std::mutex> lock(m_Mutex);
	m_UnnamedItems.erase(std::remove(m_UnnamedItems.begin(), m_UnnamedItems.end(), this), m_UnnamedItems.end());
	m_Items[m_Type].erase(m_Name);
	m_DefaultTemplates[m_Type].erase(m_Name);
}

/**
 * Retrieves a configuration item by type and name.
 *
 * @param type The type of the ConfigItem that is to be looked up.
 * @param name The name of the ConfigItem that is to be looked up.
 * @returns The configuration item.
 */
ConfigItem::Ptr ConfigItem::GetByTypeAndName(const Type::Ptr& type, const String& name)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	auto it = m_Items.find(type);

	if (it == m_Items.end())
		return nullptr;

	auto it2 = it->second.find(name);

	if (it2 == it->second.end())
		return nullptr;

	return it2->second;
}

bool ConfigItem::CommitNewItems(const ActivationContext::Ptr& context, WorkQueue& upq, std::vector<ConfigItem::Ptr>& newItems)
{
	typedef std::pair<ConfigItem::Ptr, bool> ItemPair;
	std::vector<ItemPair> items;

	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		for (const TypeMap::value_type& kv : m_Items) {
			for (const ItemMap::value_type& kv2 : kv.second) {
				if (kv2.second->m_Abstract || kv2.second->m_Object)
					continue;

				if (kv2.second->m_ActivationContext != context)
					continue;

				items.emplace_back(kv2.second, false);
			}
		}

		ItemList newUnnamedItems;

		for (const ConfigItem::Ptr& item : m_UnnamedItems) {
			if (item->m_ActivationContext != context) {
				newUnnamedItems.push_back(item);
				continue;
			}

			if (item->m_Abstract || item->m_Object)
				continue;

			items.emplace_back(item, true);
		}

		m_UnnamedItems.swap(newUnnamedItems);
	}

	if (items.empty())
		return true;

	// Shuffle all items to evenly distribute them over the threads of the workqueue. This increases perfomance
	// noticably in environments with lots of objects and available threads.
	std::shuffle(std::begin(items), std::end(items), std::default_random_engine {});

#ifdef I2_DEBUG
	Log(LogDebug, "configitem")
		<< "Committing " << items.size() << " new items.";
#endif /* I2_DEBUG */

	for (const auto& ip : items)
		newItems.push_back(ip.first);

	std::set<Type::Ptr> types;
	std::set<Type::Ptr> completed_types;

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		if (ConfigObject::TypeInstance->IsAssignableFrom(type))
			types.insert(type);
	}

	while (types.size() != completed_types.size()) {
		for (const Type::Ptr& type : types) {
			if (completed_types.find(type) != completed_types.end())
				continue;

			bool unresolved_dep = false;

			/* skip this type (for now) if there are unresolved load dependencies */
			for (const String& loadDep : type->GetLoadDependencies()) {
				Type::Ptr pLoadDep = Type::GetByName(loadDep);
				if (types.find(pLoadDep) != types.end() && completed_types.find(pLoadDep) == completed_types.end()) {
					unresolved_dep = true;
					break;
				}
			}

			if (unresolved_dep)
				continue;

			int committed_items = 0;
			upq.ParallelFor(items, [&type, &committed_items](const ItemPair& ip) {
				const ConfigItem::Ptr& item = ip.first;

				if (item->m_Type != type)
					return;

				ip.first->Commit(ip.second);
				committed_items++;
			});

			upq.Join();

			completed_types.insert(type);

#ifdef I2_DEBUG
			if (committed_items > 0)
				Log(LogDebug, "configitem")
					<< "Committed " << committed_items << " items of type '" << type->GetName() << "'.";
#endif /* I2_DEBUG */

			if (upq.HasExceptions())
				return false;
		}
	}

#ifdef I2_DEBUG
	Log(LogDebug, "configitem")
		<< "Committed " << items.size() << " items.";
#endif /* I2_DEBUG */

	completed_types.clear();

	while (types.size() != completed_types.size()) {
		for (const Type::Ptr& type : types) {
			if (completed_types.find(type) != completed_types.end())
				continue;

			bool unresolved_dep = false;

			/* skip this type (for now) if there are unresolved load dependencies */
			for (const String& loadDep : type->GetLoadDependencies()) {
				Type::Ptr pLoadDep = Type::GetByName(loadDep);
				if (types.find(pLoadDep) != types.end() && completed_types.find(pLoadDep) == completed_types.end()) {
					unresolved_dep = true;
					break;
				}
			}

			if (unresolved_dep)
				continue;

			int notified_items = 0;
			upq.ParallelFor(items, [&type, &notified_items](const ItemPair& ip) {
				const ConfigItem::Ptr& item = ip.first;

				if (!item->m_Object || item->m_Type != type)
					return;

				try {
					item->m_Object->OnAllConfigLoaded();
					notified_items++;
				} catch (const std::exception& ex) {
					if (!item->m_IgnoreOnError)
						throw;

					Log(LogNotice, "ConfigObject")
						<< "Ignoring config object '" << item->m_Name << "' of type '" << item->m_Type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

					item->Unregister();

					{
						std::unique_lock<std::mutex> lock(item->m_Mutex);
						item->m_IgnoredItems.push_back(item->m_DebugInfo.Path);
					}
				}
			});

			completed_types.insert(type);

			upq.Join();

#ifdef I2_DEBUG
			if (notified_items > 0)
				Log(LogDebug, "configitem")
					<< "Sent OnAllConfigLoaded to " << notified_items << " items of type '" << type->GetName() << "'.";
#endif /* I2_DEBUG */

			if (upq.HasExceptions())
				return false;

			notified_items = 0;
			for (const String& loadDep : type->GetLoadDependencies()) {
				upq.ParallelFor(items, [loadDep, &type, &notified_items](const ItemPair& ip) {
					const ConfigItem::Ptr& item = ip.first;

					if (!item->m_Object || item->m_Type->GetName() != loadDep)
						return;

					ActivationScope ascope(item->m_ActivationContext);
					item->m_Object->CreateChildObjects(type);
					notified_items++;
				});
			}

			upq.Join();

#ifdef I2_DEBUG
			if (notified_items > 0)
				Log(LogDebug, "configitem")
					<< "Sent CreateChildObjects to " << notified_items << " items of type '" << type->GetName() << "'.";
#endif /* I2_DEBUG */

			if (upq.HasExceptions())
				return false;

			// Make sure to activate any additionally generated items
			if (!CommitNewItems(context, upq, newItems))
				return false;
		}
	}

	return true;
}

bool ConfigItem::CommitItems(const ActivationContext::Ptr& context, WorkQueue& upq, std::vector<ConfigItem::Ptr>& newItems, bool silent)
{
	if (!silent)
		Log(LogInformation, "ConfigItem", "Committing config item(s).");

	if (!CommitNewItems(context, upq, newItems)) {
		upq.ReportExceptions("config");

		for (const ConfigItem::Ptr& item : newItems) {
			item->Unregister();
		}

		return false;
	}

	ApplyRule::CheckMatches(silent);

	if (!silent) {
		/* log stats for external parsers */
		typedef std::map<Type::Ptr, int> ItemCountMap;
		ItemCountMap itemCounts;
		for (const ConfigItem::Ptr& item : newItems) {
			if (!item->m_Object)
				continue;

			itemCounts[item->m_Object->GetReflectionType()]++;
		}

		for (const ItemCountMap::value_type& kv : itemCounts) {
			Log(LogInformation, "ConfigItem")
				<< "Instantiated " << kv.second << " " << (kv.second != 1 ? kv.first->GetPluralName() : kv.first->GetName()) << ".";
		}
	}

	return true;
}

bool ConfigItem::ActivateItems(const std::vector<ConfigItem::Ptr>& newItems, bool runtimeCreated,
	bool silent, bool withModAttrs, const Value& cookie)
{
	static std::mutex mtx;
	std::unique_lock<std::mutex> lock(mtx);

	if (withModAttrs) {
		/* restore modified attributes */
		if (Utility::PathExists(Configuration::ModAttrPath)) {
			std::unique_ptr<Expression> expression = ConfigCompiler::CompileFile(Configuration::ModAttrPath);

			if (expression) {
				try {
					ScriptFrame frame(true);
					expression->Evaluate(frame);
				} catch (const std::exception& ex) {
					Log(LogCritical, "config", DiagnosticInformation(ex));
				}
			}
		}
	}

	for (const ConfigItem::Ptr& item : newItems) {
		if (!item->m_Object)
			continue;

		ConfigObject::Ptr object = item->m_Object;

		if (object->IsActive())
			continue;

#ifdef I2_DEBUG
		Log(LogDebug, "ConfigItem")
			<< "Setting 'active' to true for object '" << object->GetName() << "' of type '" << object->GetReflectionType()->GetName() << "'";
#endif /* I2_DEBUG */

		object->PreActivate();
	}

	if (!silent)
		Log(LogInformation, "ConfigItem", "Triggering Start signal for config items");

	/* Activate objects in priority order. */
	std::vector<Type::Ptr> types = Type::GetAllTypes();

	std::sort(types.begin(), types.end(), [](const Type::Ptr& a, const Type::Ptr& b) {
		if (a->GetActivationPriority() < b->GetActivationPriority())
			return true;
		return false;
	});

	for (const Type::Ptr& type : types) {
		for (const ConfigItem::Ptr& item : newItems) {
			if (!item->m_Object)
				continue;

			ConfigObject::Ptr object = item->m_Object;
			Type::Ptr objectType = object->GetReflectionType();

			if (objectType != type)
				continue;

#ifdef I2_DEBUG
			Log(LogDebug, "ConfigItem")
				<< "Activating object '" << object->GetName() << "' of type '"
				<< objectType->GetName() << "' with priority "
				<< objectType->GetActivationPriority();
#endif /* I2_DEBUG */

			object->Activate(runtimeCreated, cookie);
		}
	}

	if (!silent)
		Log(LogInformation, "ConfigItem", "Activated all objects.");

	return true;
}

bool ConfigItem::RunWithActivationContext(const Function::Ptr& function)
{
	ActivationScope scope;

	if (!function)
		BOOST_THROW_EXCEPTION(ScriptError("'function' argument must not be null."));

	function->Invoke();

	WorkQueue upq(25000, Configuration::Concurrency);
	upq.SetName("ConfigItem::RunWithActivationContext");

	std::vector<ConfigItem::Ptr> newItems;

	if (!CommitItems(scope.GetContext(), upq, newItems, true))
		return false;

	if (!ActivateItems(newItems, false, true))
		return false;

	return true;
}

std::vector<ConfigItem::Ptr> ConfigItem::GetItems(const Type::Ptr& type)
{
	std::vector<ConfigItem::Ptr> items;

	std::unique_lock<std::mutex> lock(m_Mutex);

	auto it = m_Items.find(type);

	if (it == m_Items.end())
		return items;

	items.reserve(it->second.size());

	for (const ItemMap::value_type& kv : it->second) {
		items.push_back(kv.second);
	}

	return items;
}

std::vector<ConfigItem::Ptr> ConfigItem::GetDefaultTemplates(const Type::Ptr& type)
{
	std::vector<ConfigItem::Ptr> items;

	std::unique_lock<std::mutex> lock(m_Mutex);

	auto it = m_DefaultTemplates.find(type);

	if (it == m_DefaultTemplates.end())
		return items;

	items.reserve(it->second.size());

	for (const ItemMap::value_type& kv : it->second) {
		items.push_back(kv.second);
	}

	return items;
}

void ConfigItem::RemoveIgnoredItems(const String& allowedConfigPath)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	for (const String& path : m_IgnoredItems) {
		if (path.Find(allowedConfigPath) == String::NPos)
			continue;

		Log(LogNotice, "ConfigItem")
			<< "Removing ignored item path '" << path << "'.";

		(void) unlink(path.CStr());
	}

	m_IgnoredItems.clear();
}
