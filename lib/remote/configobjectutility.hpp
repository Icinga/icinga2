/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	static String GetObjectConfigPath(const Type::Ptr& type, const String& fullName);
	static void RepairPackage(const String& package);
	static void CreateStorage();

	static String CreateObjectConfig(const Type::Ptr& type, const String& fullName,
		bool ignoreOnError, const Array::Ptr& templates, const Dictionary::Ptr& attrs);

	static bool CreateObject(const Type::Ptr& type, const String& fullName,
		const String& config, const Array::Ptr& errors, const Array::Ptr& diagnosticInformation);

	static bool DeleteObject(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors,
		const Array::Ptr& diagnosticInformation);

private:
	static String EscapeName(const String& name);
	static bool DeleteObjectHelper(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors,
		const Array::Ptr& diagnosticInformation);
};

}

#endif /* CONFIGOBJECTUTILITY_H */
