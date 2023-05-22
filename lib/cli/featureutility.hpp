/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "cli/i2-cli.hpp"
#include "base/string.hpp"
#include <vector>
#include <iostream>

namespace icinga
{

/**
 * @ingroup cli
 */
class FeatureUtility
{
public:
	static String GetFeaturesAvailablePath();
	static String GetFeaturesEnabledPath();

	static std::vector<String> GetFieldCompletionSuggestions(const String& word, bool enable);

	static int EnableFeatures(const std::vector<std::string>& features);
	static int DisableFeatures(const std::vector<std::string>& features);
	static int ListFeatures(std::ostream& os = std::cout);

	static bool GetFeatures(std::vector<String>& features, bool enable);
	static bool CheckFeatureEnabled(const String& feature);
	static bool CheckFeatureDisabled(const String& feature);

private:
	FeatureUtility();
	static void CollectFeatures(const String& feature_file, std::vector<String>& features);
	static bool CheckFeatureInternal(const String& feature, bool check_disabled);
};

}
