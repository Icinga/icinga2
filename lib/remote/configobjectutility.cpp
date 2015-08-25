/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "remote/configobjectutility.hpp"
#include "remote/configmoduleutility.hpp"
#include "config/configitembuilder.hpp"
#include "config/configitem.hpp"
#include "config/configwriter.hpp"
#include "base/exception.hpp"
#include "base/serializer.hpp"
#include "base/dependencygraph.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace icinga;

String ConfigObjectUtility::GetConfigDir(void)
{
	return ConfigModuleUtility::GetModuleDir() + "/_api/" +
	    ConfigModuleUtility::GetActiveStage("_api");
}

String ConfigObjectUtility::EscapeName(const String& name)
{
	return Utility::EscapeString(name, "<>:\"/\\|?*", true);
}

bool ConfigObjectUtility::CreateObject(const Type::Ptr& type, const String& fullName,
    const Array::Ptr& templates, const Dictionary::Ptr& attrs, const Array::Ptr& errors)
{
	NameComposer *nc = dynamic_cast<NameComposer *>(type.get());
	Dictionary::Ptr nameParts;
	String name;

	if (nc) {
		nameParts = nc->ParseName(fullName);
		name = nameParts->Get("name");
	} else
		name = fullName;

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder();
	builder->SetType(type->GetName());
	builder->SetName(name);
	builder->SetScope(ScriptGlobal::GetGlobals());
	builder->SetModule("_api");

	if (templates) {
		ObjectLock olock(templates);
		BOOST_FOREACH(const String& tmpl, templates) {
			ImportExpression *expr = new ImportExpression(MakeLiteral(tmpl));
			builder->AddExpression(expr);
		}
	}

	if (nameParts) {
		ObjectLock olock(nameParts);
		BOOST_FOREACH(const Dictionary::Pair& kv, nameParts) {
			SetExpression *expr = new SetExpression(MakeIndexer(ScopeThis, kv.first), OpSetLiteral, MakeLiteral(kv.second));
			builder->AddExpression(expr);
		}
	}

	if (attrs) {
		ObjectLock olock(attrs);
		BOOST_FOREACH(const Dictionary::Pair& kv, attrs) {
			std::vector<String> tokens;
			boost::algorithm::split(tokens, kv.first, boost::is_any_of("."));
			
			Expression *expr = new GetScopeExpression(ScopeThis);
			
			BOOST_FOREACH(const String& val, tokens) {
				expr = new IndexerExpression(expr, MakeLiteral(val));
			}
			
			SetExpression *aexpr = new SetExpression(expr, OpSetLiteral, MakeLiteral(kv.second));
			builder->AddExpression(aexpr);
		}
	}
	
	try {
		ConfigItem::Ptr item = builder->Compile();
		item->Register();

		WorkQueue upq;

		if (!ConfigItem::CommitItems(upq) || !ConfigItem::ActivateItems(upq, false)) {
			if (errors) {
				BOOST_FOREACH(const boost::exception_ptr& ex, upq.GetExceptions()) {
					errors->Add(DiagnosticInformation(ex));
				}
			}
			
			return false;
		}
	} catch (const std::exception& ex) {
		if (errors)
			errors->Add(DiagnosticInformation(ex));
			
		return false;
	}
	
	if (!ConfigModuleUtility::ModuleExists("_api")) {
		ConfigModuleUtility::CreateModule("_api");
	
		String stage = ConfigModuleUtility::CreateStage("_api");
		ConfigModuleUtility::ActivateStage("_api", stage);
	} 
	
	String typeDir = type->GetPluralName();
	boost::algorithm::to_lower(typeDir);
	
	String path = GetConfigDir() + "/conf.d/" + typeDir;
	Utility::MkDirP(path, 0700);

	path += "/" + EscapeName(fullName) + ".conf";
	
	Dictionary::Ptr allAttrs = new Dictionary();
	attrs->CopyTo(allAttrs);

	if (nameParts)
		nameParts->CopyTo(allAttrs);

	allAttrs->Remove("name");

	ConfigWriter::Ptr cw = new ConfigWriter(path);
	cw->EmitConfigItem(type->GetName(), name, false, templates, allAttrs);
	cw->EmitRaw("\n");
	
	return true;
}

bool ConfigObjectUtility::DeleteObjectHelper(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors)
{
	std::vector<Object::Ptr> parents = DependencyGraph::GetParents(object);

	if (!parents.empty() && !cascade) {
		if (errors)
			errors->Add("Object cannot be deleted because other objects depend on it. Use cascading delete to delete it anyway.");

		return false;
	}

	BOOST_FOREACH(const Object::Ptr& pobj, parents) {
		ConfigObject::Ptr parentObj = dynamic_pointer_cast<ConfigObject>(pobj);

		if (!parentObj)
			continue;

		DeleteObjectHelper(parentObj, cascade, errors);
	}

	Type::Ptr type = object->GetReflectionType();

	ConfigItem::Ptr item = ConfigItem::GetObject(type->GetName(), object->GetName());

	try {
		object->Deactivate();

		if (item)
			item->Unregister();
		else
			object->Unregister();

	} catch (const std::exception& ex) {
		if (errors)
			errors->Add(DiagnosticInformation(ex));
			
		return false;
	}

	String typeDir = type->GetPluralName();
	boost::algorithm::to_lower(typeDir);
	
	String path = GetConfigDir() + "/conf.d/" + typeDir +
	    "/" + EscapeName(object->GetName()) + ".conf";
	
	if (Utility::PathExists(path)) {
		if (unlink(path.CStr()) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("unlink")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(path));
		}
	}

	return true;
}

bool ConfigObjectUtility::DeleteObject(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors)
{
	if (object->GetModule() != "_api") {
		if (errors)
			errors->Add("Object cannot be deleted because it was not created using the API.");

		return false;
	}

	return DeleteObjectHelper(object, cascade, errors);
}

