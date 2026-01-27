// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGOBJECTUTILITY_H
#define CONFIGOBJECTUTILITY_H

#include "remote/i2-remote.hpp"
#include "base/array.hpp"
#include "base/configobject.hpp"
#include "base/dictionary.hpp"
#include "base/type.hpp"

namespace icinga
{

/**
 * Helper functions.
 *
 * @ingroup remote
 */
class ConfigObjectUtility
{

public:
	static String GetConfigDir();
	static String ComputeNewObjectConfigPath(const Type::Ptr& type, const String& fullName);
	static String GetExistingObjectConfigPath(const ConfigObject::Ptr& object);
	static void RepairPackage(const String& package);
	static void CreateStorage();

	static String CreateObjectConfig(const Type::Ptr& type, const String& fullName,
		bool ignoreOnError, const Array::Ptr& templates, const Dictionary::Ptr& attrs);

	static bool CreateObject(const Type::Ptr& type, const String& fullName,
		const String& config, const Array::Ptr& errors, const Array::Ptr& diagnosticInformation, const Value& cookie = Empty);

	static bool DeleteObject(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors,
		const Array::Ptr& diagnosticInformation, const Value& cookie = Empty);

private:
	static String EscapeName(const String& name);
	static bool DeleteObjectHelper(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors,
		const Array::Ptr& diagnosticInformation, const Value& cookie = Empty);
};

}

#endif /* CONFIGOBJECTUTILITY_H */
