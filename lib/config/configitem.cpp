/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include <sstream>
#include <fstream>

using namespace icinga;

boost::mutex ConfigItem::m_Mutex;
ConfigItem::TypeMap ConfigItem::m_Items;
ConfigItem::TypeMap ConfigItem::m_DefaultTemplates;
ConfigItem::ItemList ConfigItem::m_UnnamedItems;
ConfigItem::IgnoredItemList ConfigItem::m_IgnoredItems;

REGISTER_SCRIPTFUNCTION_NS(Internal, run_with_activation_context, &ConfigItem::RunWithActivationContext, "func");

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
ConfigItem::ConfigItem(const Type::Ptr& type, const String& name,
    bool abstract, const std::shared_ptr<Expression>& exprl,
    const std::shared_ptr<Expression>& filter, bool defaultTmpl, bool ignoreOnError,
    const DebugInfo& debuginfo, const Dictionary::Ptr& scope,
    const String& zone, const String& package)
	: m_Type(type), m_Name(name), m_Abstract(abstract),
	  m_Expression(exprl), m_Filter(filter),
	  m_DefaultTmpl(defaultTmpl), m_IgnoreOnError(ignoreOnError),
	  m_DebugInfo(debuginfo), m_Scope(scope), m_Zone(zone),
	  m_Package(package)
{
}

/**
 * Retrieves the type of the configuration item.
 *
 * @returns The type.
 */
Type::Ptr ConfigItem::GetType(void) const
{
	return m_Type;
}

/**
 * Retrieves the name of the configuration item.
 *
 * @returns The name.
 */
String ConfigItem::GetName(void) const
{
	return m_Name;
}

/**
 * Checks whether the item is abstract.
 *
 * @returns true if the item is abstract, false otherwise.
 */
bool ConfigItem::IsAbstract(void) const
{
	return m_Abstract;
}

bool ConfigItem::IsDefaultTemplate(void) const
{
	return m_DefaultTmpl;
}

bool ConfigItem::IsIgnoreOnError(void) const
{
	return m_IgnoreOnError;
}

/**
 * Retrieves the debug information for the configuration item.
 *
 * @returns The debug information.
 */
DebugInfo ConfigItem::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Dictionary::Ptr ConfigItem::GetScope(void) const
{
	return m_Scope;
}

ConfigObject::Ptr ConfigItem::GetObject(void) const
{
	return m_Object;
}

/**
 * Retrieves the expression list for the configuration item.
 *
 * @returns The expression list.
 */
std::shared_ptr<Expression> ConfigItem::GetExpression(void) const
{
	return m_Expression;
}

/**
* Retrieves the object filter for the configuration item.
*
* @returns The filter expression.
*/
std::shared_ptr<Expression> ConfigItem::GetFilter(void) const
{
	return m_Filter;
}

class DefaultValidationUtils : public ValidationUtils
{
public:
	virtual bool ValidateName(const String& type, const String& name) const override
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
#ifdef I2_DEBUG
	Log(LogDebug, "ConfigItem")
	    << "Commit called for ConfigItem Type=" << GetType() << ", Name=" << GetName();
#endif /* I2_DEBUG */

	/* Make sure the type is valid. */
	Type::Ptr type = GetType();
	if (!type || !ConfigObject::TypeInstance->IsAssignableFrom(type))
		BOOST_THROW_EXCEPTION(ScriptError("Type '" + GetType() + "' does not exist.", m_DebugInfo));

	if (IsAbstract())
		return nullptr;

	ConfigObject::Ptr dobj = static_pointer_cast<ConfigObject>(type->Instantiate(std::vector<Value>()));

	dobj->SetDebugInfo(m_DebugInfo);
	dobj->SetZoneName(m_Zone);
	dobj->SetPackage(m_Package);
	dobj->SetName(m_Name);

	DebugHint debugHints;

	ScriptFrame frame(dobj);
	if (m_Scope)
		m_Scope->CopyTo(frame.Locals);
	try {
		m_Expression->Evaluate(frame, &debugHints);
	} catch (const std::exception& ex) {
		if (m_IgnoreOnError) {
			Log(LogNotice, "ConfigObject")
			    << "Ignoring config object '" << m_Name << "' of type '" << m_Type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

			{
				boost::mutex::scoped_lock lock(m_Mutex);
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

	NameComposer *nc = dynamic_cast<NameComposer *>(type.get());

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
			    << "Ignoring config object '" << m_Name << "' of type '" << m_Type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

			{
				boost::mutex::scoped_lock lock(m_Mutex);
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
				boost::mutex::scoped_lock lock(m_Mutex);
				m_IgnoredItems.push_back(m_DebugInfo.Path);
			}

			return nullptr;
		}

		throw;
	}

	Dictionary::Ptr persistentItem = new Dictionary();

	persistentItem->Set("type", GetType()->GetName());
	persistentItem->Set("name", GetName());
	persistentItem->Set("properties", Serialize(dobj, FAConfig));
	persistentItem->Set("debug_hints", dhint);

	Array::Ptr di = new Array();
	di->Add(m_DebugInfo.Path);
	di->Add(m_DebugInfo.FirstLine);
	di->Add(m_DebugInfo.FirstColumn);
	di->Add(m_DebugInfo.LastLine);
	di->Add(m_DebugInfo.LastColumn);
	persistentItem->Set("debug_info", di);

	ConfigCompilerContext::GetInstance()->WriteObject(persistentItem);
	persistentItem.reset();

	dhint.reset();

	dobj->Register();

	m_Object = dobj;

	return dobj;
}

/**
 * Registers the configuration item.
 */
void ConfigItem::Register(void)
{
	m_ActivationContext = ActivationContext::GetCurrentContext();

	boost::mutex::scoped_lock lock(m_Mutex);

	/* If this is a non-abstract object with a composite name
	 * we register it in m_UnnamedItems instead of m_Items. */
	if (!m_Abstract && dynamic_cast<NameComposer *>(m_Type.get()))
		m_UnnamedItems.push_back(this);
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
void ConfigItem::Unregister(void)
{
	if (m_Object) {
		m_Object->Unregister();
		m_Object.reset();
	}

	boost::mutex::scoped_lock lock(m_Mutex);
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
	boost::mutex::scoped_lock lock(m_Mutex);

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
		boost::mutex::scoped_lock lock(m_Mutex);

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

	for (const ItemPair& ip : items) {
		newItems.push_back(ip.first);
		upq.Enqueue([&]() {
			ip.first->Commit(ip.second);
		});
	}

	upq.Join();

	if (upq.HasExceptions())
		return false;

	std::set<Type::Ptr> types;

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		if (ConfigObject::TypeInstance->IsAssignableFrom(type))
			types.insert(type);
	}

	std::set<Type::Ptr> completed_types;

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

			for (const ItemPair& ip : items) {
				const ConfigItem::Ptr& item = ip.first;

				if (!item->m_Object)
					continue;

				if (item->m_Type == type) {
					upq.Enqueue([&]() {
						try {
							item->m_Object->OnAllConfigLoaded();
						} catch (const std::exception& ex) {
							if (item->m_IgnoreOnError) {
								Log(LogNotice, "ConfigObject")
								    << "Ignoring config object '" << item->m_Name << "' of type '" << item->m_Type->GetName() << "' due to errors: " << DiagnosticInformation(ex);

								item->Unregister();

								{
									boost::mutex::scoped_lock lock(item->m_Mutex);
									item->m_IgnoredItems.push_back(item->m_DebugInfo.Path);
								}

								return;
							}

							throw;
						}
					});
				}
			}

			completed_types.insert(type);

			upq.Join();

			if (upq.HasExceptions())
				return false;

			for (const String& loadDep : type->GetLoadDependencies()) {
				for (const ItemPair& ip : items) {
					const ConfigItem::Ptr& item = ip.first;

					if (!item->m_Object)
						continue;

					if (item->m_Type->GetName() == loadDep) {
						upq.Enqueue([&]() {
							ActivationScope ascope(item->m_ActivationContext);
							item->m_Object->CreateChildObjects(type);
						});
					}
				}
			}

			upq.Join();

			if (upq.HasExceptions())
				return false;

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

	ApplyRule::CheckMatches();

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

bool ConfigItem::ActivateItems(WorkQueue& upq, const std::vector<ConfigItem::Ptr>& newItems, bool runtimeCreated, bool silent, bool withModAttrs)
{
	static boost::mutex mtx;
	boost::mutex::scoped_lock lock(mtx);

	if (withModAttrs) {
		/* restore modified attributes */
		if (Utility::PathExists(Application::GetModAttrPath())) {
			Expression *expression = ConfigCompiler::CompileFile(Application::GetModAttrPath());

			if (expression) {
				try {
					ScriptFrame frame;
					expression->Evaluate(frame);
				} catch (const std::exception& ex) {
					Log(LogCritical, "config", DiagnosticInformation(ex));
				}
			}

			delete expression;
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
		upq.Enqueue(std::bind(&ConfigObject::PreActivate, object));
	}

	upq.Join();

	if (upq.HasExceptions()) {
		upq.ReportExceptions("ConfigItem");
		return false;
	}

	if (!silent)
		Log(LogInformation, "ConfigItem", "Triggering Start signal for config items");

	for (const ConfigItem::Ptr& item : newItems) {
		if (!item->m_Object)
			continue;

		ConfigObject::Ptr object = item->m_Object;

#ifdef I2_DEBUG
		Log(LogDebug, "ConfigItem")
		    << "Activating object '" << object->GetName() << "' of type '" << object->GetReflectionType()->GetName() << "'";
#endif /* I2_DEBUG */
		upq.Enqueue(std::bind(&ConfigObject::Activate, object, runtimeCreated));
	}

	upq.Join();

	if (upq.HasExceptions()) {
		upq.ReportExceptions("ConfigItem");
		return false;
	}

#ifdef I2_DEBUG
	for (const ConfigItem::Ptr& item : newItems) {
		ConfigObject::Ptr object = item->m_Object;

		if (!object)
			continue;

		ASSERT(object && object->IsActive());
	}
#endif /* I2_DEBUG */

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

	WorkQueue upq(25000, Application::GetConcurrency());
	upq.SetName("ConfigItem::RunWithActivationContext");

	std::vector<ConfigItem::Ptr> newItems;

	if (!CommitItems(scope.GetContext(), upq, newItems, true))
		return false;

	if (!ActivateItems(upq, newItems, false, true))
		return false;

	return true;
}

std::vector<ConfigItem::Ptr> ConfigItem::GetItems(const Type::Ptr& type)
{
	std::vector<ConfigItem::Ptr> items;

	boost::mutex::scoped_lock lock(m_Mutex);

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

	boost::mutex::scoped_lock lock(m_Mutex);

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
	boost::mutex::scoped_lock lock(m_Mutex);

	for (const String& path : m_IgnoredItems) {
		if (path.Find(allowedConfigPath) == String::NPos)
			continue;

		Log(LogNotice, "ConfigItem")
		    << "Removing ignored item path '" << path << "'.";

		(void) unlink(path.CStr());
	}

	m_IgnoredItems.clear();
}
