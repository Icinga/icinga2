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

#ifndef FEATUREUTILITY_H
#define FEATUREUTILITY_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <vector>

namespace icinga
{

/**
 * @ingroup cli
 */
class FeatureUtility
{
public:
	static String GetFeaturesAvailablePath(void);
	static String GetFeaturesEnabledPath(void);

	static std::vector<String> GetFieldCompletionSuggestions(const String& word, bool enable);

	static int EnableFeatures(const std::vector<std::string>& features);
	static int DisableFeatures(const std::vector<std::string>& features);
	static int ListFeatures(std::ostream& os = std::cout);

	static bool GetFeatures(std::vector<String>& features, bool enable);

private:
	FeatureUtility(void);
	static void CollectFeatures(const String& feature_file, std::vector<String>& features);
};

}

#endif /* FEATUREUTILITY_H */
