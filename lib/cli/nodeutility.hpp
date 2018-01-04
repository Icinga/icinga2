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

#ifndef NODEUTILITY_H
#define NODEUTILITY_H

#include "base/i2-base.hpp"
#include "cli/i2-cli.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/value.hpp"
#include "base/string.hpp"
#include <vector>

namespace icinga
{

/**
 * @ingroup cli
 */
class NodeUtility
{
public:
	static String GetConstantsConfPath();

	static bool CreateBackupFile(const String& target, bool is_private = false);

	static bool WriteNodeConfigObjects(const String& filename, const Array::Ptr& objects);

	static void UpdateConstant(const String& name, const String& value);

	/* node setup helpers */
	static int GenerateNodeIcingaConfig(const std::vector<std::string>& endpoints, const std::vector<String>& globalZones);
	static int GenerateNodeMasterIcingaConfig(const std::vector<String>& globalZones);

private:
	NodeUtility();

	static void SerializeObject(std::ostream& fp, const Dictionary::Ptr& object);
};

}

#endif /* NODEUTILITY_H */
