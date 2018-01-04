/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

	static String CreateObjectConfig(const Type::Ptr& type, const String& fullName,
		bool ignoreOnError, const Array::Ptr& templates, const Dictionary::Ptr& attrs);

	static bool CreateObject(const Type::Ptr& type, const String& fullName,
		const String& config, const Array::Ptr& errors);

	static bool DeleteObject(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors);

private:
	static String EscapeName(const String& name);
	static bool DeleteObjectHelper(const ConfigObject::Ptr& object, bool cascade, const Array::Ptr& errors);
};

}

#endif /* CONFIGOBJECTUTILITY_H */
