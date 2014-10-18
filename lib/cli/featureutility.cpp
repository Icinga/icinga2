/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/featureutility.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

std::vector<String> FeatureUtility::GetFieldCompletionSuggestions(FeatureCommandType fctype, const String& word)
{
	std::vector<String> cache;
	std::vector<String> suggestions;

	if (fctype == FeatureCommandEnable) {
		/* only suggest features not already enabled */
		GetFeatures(FeaturesDisabled, cache);
	} else if (fctype == FeatureCommandDisable) {
		/* suggest all enabled features */
		GetFeatures(FeaturesEnabled, cache);
	}

	std::sort(cache.begin(), cache.end());

	BOOST_FOREACH(const String& suggestion, cache) {
		if (suggestion.Find(word) == 0)
			suggestions.push_back(suggestion);
	}

	return suggestions;
}

bool FeatureUtility::GetFeatures(FeatureType ftype, std::vector<String>& features)
{
	String path = Application::GetSysconfDir() + "/icinga2/";

	/* disabled = available-enabled */
	if (ftype == FeaturesDisabled) {
		std::vector<String> enabled;
		std::vector<String> available;
		GetFeatures(FeaturesAvailable, available);
		GetFeatures(FeaturesEnabled, enabled);

		std::sort(available.begin(), available.end());
		std::sort(enabled.begin(), enabled.end());
		std::set_difference(
			available.begin(), available.end(),
			enabled.begin(), enabled.end(),
			std::back_inserter(features)
		);

		return true;
	} else {
		if (ftype == FeaturesAvailable)
			path += "features-available/";
		else if (ftype == FeaturesEnabled)
			path += "features-enabled/";
		else {
			Log(LogCritical, "cli", "Unknown feature type passed. Bailing out.");
			return false;
		}

		if (!Utility::Glob(path + "/*.conf",
		    boost::bind(&FeatureUtility::CollectFeatures, _1, boost::ref(features)), GlobFile)) {
			Log(LogCritical, "cli", "Cannot access path '" + path + "'.");
			return false;
		}
	}

	return true;
}

void FeatureUtility::CollectFeatures(const String& feature_file, std::vector<String>& features)
{
	String feature = Utility::BaseName(feature_file);
	boost::algorithm::replace_all(feature, ".conf", "");

	Log(LogDebug, "cli", "Adding feature: " + feature);
	features.push_back(feature);
}
