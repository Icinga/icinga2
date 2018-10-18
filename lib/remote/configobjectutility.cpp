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

#include "remote/configobjectutility.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/apilistener.hpp"
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "base/configwriter.hpp"
#include "base/exception.hpp"
#include "base/dependencygraph.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>

using namespace icinga;

String ConfigObjectUtility::GetConfigDir()
{
	return ConfigPackageUtility::GetPackageDir() + "/_api/" +
		ConfigPackageUtility::GetActiveStage("_api");
}

String ConfigObjectUtility::GetObjectConfigPath(const Type::Ptr& type, const String& fullName)
{
	String typeDir = type->GetPluralName();
	boost::algorithm::to_lower(typeDir);

	return GetConfigDir() + "/conf.d/" + typeDir +
		"/" + EscapeName(fullName) + ".conf";
}

String ConfigObjectUtility::EscapeName(const String& name)
{
	return Utility::EscapeString(name, "<>:\"/\\|?*", true);
}

String ConfigObjectUtility::CreateObjectConfig(const Type::Ptr& type, const String& fullName,
	bool ignoreOnError, const Array::Ptr& templates, const Dictionary::Ptr& attrs)
{
	auto *nc = dynamic_cast<NameComposer *>(type.get());
	Dictionary::Ptr nameParts;
	String name;

	if (nc) {
		nameParts = nc->ParseName(fullName);
		name = nameParts->Get("name");
	} else
		name = fullName;

	Dictionary::Ptr allAttrs = new Dictionary();

	if (attrs) {
		attrs->CopyTo(allAttrs);

		ObjectLock olock(attrs);
		for (const Dictionary::Pair& kv : attrs) {
			int fid = type->GetFieldId(kv.first.SubStr(0, kv.first.FindFirstOf(".")));

			if (fid < 0)
				BOOST_THROW_EXCEPTION(ScriptError("Invalid attribute specified: " + kv.first));

			Field field = type->GetFieldInfo(fid);

			if (!(field.Attributes & FAConfig) || kv.first == "name")
				BOOST_THROW_EXCEPTION(ScriptError("Attribute is marked for internal use only and may not be set: " + kv.first));
		}
	}

	if (nameParts)
		nameParts->CopyTo(allAttrs);

	allAttrs->Remove("name");

	/* update the version for config sync */
	allAttrs->Set("version", Utility::GetTime());

	std::ostringstream config;
	ConfigWriter::EmitConfigItem(config, type->GetName(), name, false, ignoreOnError, templates, allAttrs);
	ConfigWriter::EmitRaw(config, "\n");

	return config.str();
}

bool ConfigObjectUtility::CreateObject(const Type::Ptr& type, const String& fullName,
	const String& config, const Array::Ptr& errors, const Array::Ptr& diagnosticInformation)
{
	{
		boost::mutex::scoped_lock lock(ConfigPackageUtility::GetStaticMutex());
		if (!ConfigPackageUtility::PackageExists("_api")) {
			ConfigPackageUtility::CreatePackage("_api");

			String stage = ConfigPackageUtility::CreateStage("_api");
			ConfigPackageUtility::ActivateStage("_api", stage);
		}
	}

	ConfigItem::Ptr item = ConfigItem::GetByTypeAndName(type, fullName);

	if (item) {
		errors->Add("Object '" + fullName + "' already exists.");
		return false;
	}

	String path = GetObjectConfigPath(type, fullName);
	Utility::MkDirP(Utility::DirName(path), 0700);

	if (Utility::PathExists(path)) {
		errors->Add("Cannot create object '" + fullName + "'. Configuration file '" + path + "' already exists.");
		return false;
	}

	std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::trunc);
	fp << config;
	fp.close();

	std::unique_ptr<Expression> expr = ConfigCompiler::CompileFile(path, String(), "_api");

	try {
		ActivationScope ascope;

		ScriptFrame frame(true);
		expr->Evaluate(frame);
		expr.reset();

		WorkQueue upq;
		upq.SetName("ConfigObjectUtility::CreateObject");

		std::vector<ConfigItem::Ptr> newItems;

		/* Disable logging for object creation, but do so ourselves later on. */
		if (!ConfigItem::CommitItems(ascope.GetContext(), upq, newItems, true) || !ConfigItem::ActivateItems(upq, newItems, true, true)) {
			if (errors) {
				if (unlink(path.CStr()) < 0 && errno != ENOENT) {
					BOOST_THROW_EXCEPTION(posix_error()
						<< boost::errinfo_api_function("unlink")
						<< boost::errinfo_errno(errno)
						<< boost::errinfo_file_name(path));
				}

				for (const boost::exception_ptr& ex : upq.GetExceptions()) {
					errors->Add(DiagnosticInformation(ex, false));

					if (diagnosticInformation)
						diagnosticInformation->Add(DiagnosticInformation(ex));
				}
			}

			return false;
		}

		ApiListener::UpdateObjectAuthority();

		Log(LogInformation, "ConfigObjectUtility")
			<< "Created and activated object '" << fullName << "' of type '" << type->GetName() << "'.";

	} catch (const std::exception& ex) {
		if (unlink(path.CStr()) < 0 && errno != ENOENT) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("unlink")
				<< boost::errinfo_errno(errno)
				<< boost::errinfo_file_name(path));
		}

		if (errors)
			errors->Add(DiagnosticInformation(ex, false));

		if (diagnosticInformation)
			diagnosticInformation->Add(DiagnosticInformation(ex));

		return false;
	}

	return true;
}

bool ConfigObjectUtility::DeleteObjectHelper(const ConfigObject::Ptr& object, bool cascade,
	const Array::Ptr& errors, const Array::Ptr& diagnosticInformation)
{
	std::vector<Object::Ptr> parents = DependencyGraph::GetParents(object);

	Type::Ptr type = object->GetReflectionType();

	String name = object->GetName();

	if (!parents.empty() && !cascade) {
		if (errors) {
			errors->Add("Object '" + name + "' of type '" + type->GetName() +
				"' cannot be deleted because other objects depend on it. "
				"Use cascading delete to delete it anyway.");
		}

		return false;
	}

	for (const Object::Ptr& pobj : parents) {
		ConfigObject::Ptr parentObj = dynamic_pointer_cast<ConfigObject>(pobj);

		if (!parentObj)
			continue;

		DeleteObjectHelper(parentObj, cascade, errors, diagnosticInformation);
	}

	ConfigItem::Ptr item = ConfigItem::GetByTypeAndName(type, name);

	try {
		/* mark this object for cluster delete event */
		object->SetExtension("ConfigObjectDeleted", true);
		/* triggers signal for DB IDO and other interfaces */
		object->Deactivate(true);

		if (item)
			item->Unregister();
		else
			object->Unregister();

	} catch (const std::exception& ex) {
		if (errors)
			errors->Add(DiagnosticInformation(ex, false));

		if (diagnosticInformation)
			diagnosticInformation->Add(DiagnosticInformation(ex));

		return false;
	}

	String path = GetObjectConfigPath(object->GetReflectionType(), name);

	if (Utility::PathExists(path)) {
		if (unlink(path.CStr()) < 0 && errno != ENOENT) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("unlink")
				<< boost::errinfo_errno(errno)
				<< boost::errinfo_file_name(path));
		}
	}

	return true;
}

bool ConfigObjectUtility::DeleteObject(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors, const Array::Ptr& diagnosticInformation)
{
	if (object->GetPackage() != "_api") {
		if (errors)
			errors->Add("Object cannot be deleted because it was not created using the API.");

		return false;
	}

	return DeleteObjectHelper(object, cascade, errors, diagnosticInformation);
}
