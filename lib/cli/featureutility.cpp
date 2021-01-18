/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/featureutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

String FeatureUtility::GetFeaturesAvailablePath()
{
	return Configuration::ConfigDir + "/features-available";
}

String FeatureUtility::GetFeaturesEnabledPath()
{
	return Configuration::ConfigDir + "/features-enabled";
}

std::vector<String> FeatureUtility::GetFieldCompletionSuggestions(const String& word, bool enable)
{
	std::vector<String> cache;
	std::vector<String> suggestions;

	GetFeatures(cache, enable);

	std::sort(cache.begin(), cache.end());

	for (const String& suggestion : cache) {
		if (suggestion.Find(word) == 0)
			suggestions.push_back(suggestion);
	}

	return suggestions;
}

int FeatureUtility::EnableFeatures(const std::vector<std::string>& features)
{
	String features_available_dir = GetFeaturesAvailablePath();
	String features_enabled_dir = GetFeaturesEnabledPath();

	if (!Utility::PathExists(features_available_dir) ) {
		Log(LogCritical, "cli")
			<< "Cannot parse available features. Path '" << features_available_dir << "' does not exist.";
		return 1;
	}

	if (!Utility::PathExists(features_enabled_dir) ) {
		Log(LogCritical, "cli")
			<< "Cannot enable features. Path '" << features_enabled_dir << "' does not exist.";
		return 1;
	}

	std::vector<std::string> errors;

	for (const String& feature : features) {
		String source = features_available_dir + "/" + feature + ".conf";

		if (!Utility::PathExists(source) ) {
			Log(LogCritical, "cli")
				<< "Cannot enable feature '" << feature << "'. Source file '" << source + "' does not exist.";
			errors.push_back(feature);
			continue;
		}

		String target = features_enabled_dir + "/" + feature + ".conf";

		if (Utility::PathExists(target) ) {
			Log(LogWarning, "cli")
				<< "Feature '" << feature << "' already enabled.";
			continue;
		}

		std::cout << "Enabling feature " << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << feature
			<< ConsoleColorTag(Console_Normal) << ". Make sure to restart Icinga 2 for these changes to take effect.\n";

#ifndef _WIN32
		String relativeSource = "../features-available/" + feature + ".conf";

		if (symlink(relativeSource.CStr(), target.CStr()) < 0) {
			Log(LogCritical, "cli")
				<< "Cannot enable feature '" << feature << "'. Linking source '" << relativeSource << "' to target file '" << target
				<< "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\".";
			errors.push_back(feature);
			continue;
		}
#else /* _WIN32 */
		std::ofstream fp;
		fp.open(target.CStr());
		fp << "include \"../features-available/" << feature << ".conf\"" << std::endl;
		fp.close();

		if (fp.fail()) {
			Log(LogCritical, "cli")
				<< "Cannot enable feature '" << feature << "'. Failed to open file '" << target << "'.";
			errors.push_back(feature);
			continue;
		}
#endif /* _WIN32 */
	}

	if (!errors.empty()) {
		Log(LogCritical, "cli")
			<< "Cannot enable feature(s): " << boost::algorithm::join(errors, " ");
		errors.clear();
		return 1;
	}

	return 0;
}

int FeatureUtility::DisableFeatures(const std::vector<std::string>& features)
{
	String features_enabled_dir = GetFeaturesEnabledPath();

	if (!Utility::PathExists(features_enabled_dir) ) {
		Log(LogCritical, "cli")
			<< "Cannot disable features. Path '" << features_enabled_dir << "' does not exist.";
		return 0;
	}

	std::vector<std::string> errors;

	for (const String& feature : features) {
		String target = features_enabled_dir + "/" + feature + ".conf";

		if (!Utility::PathExists(target) ) {
			Log(LogWarning, "cli")
				<< "Feature '" << feature << "' already disabled.";
			continue;
		}

		if (unlink(target.CStr()) < 0) {
			Log(LogCritical, "cli")
				<< "Cannot disable feature '" << feature << "'. Unlinking target file '" << target
				<< "' failed with error code " << errno << ", \"" + Utility::FormatErrorNumber(errno) << "\".";
			errors.push_back(feature);
			continue;
		}

		std::cout << "Disabling feature " << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << feature
			<< ConsoleColorTag(Console_Normal) << ". Make sure to restart Icinga 2 for these changes to take effect.\n";
	}

	if (!errors.empty()) {
		Log(LogCritical, "cli")
			<< "Cannot disable feature(s): " << boost::algorithm::join(errors, " ");
		errors.clear();
		return 1;
	}

	return 0;
}

int FeatureUtility::ListFeatures(std::ostream& os)
{
	std::vector<String> disabled_features;
	std::vector<String> enabled_features;

	if (!FeatureUtility::GetFeatures(disabled_features, true))
		return 1;

	os << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "Disabled features: " << ConsoleColorTag(Console_Normal)
		<< boost::algorithm::join(disabled_features, " ") << "\n";

	if (!FeatureUtility::GetFeatures(enabled_features, false))
		return 1;

	os << ConsoleColorTag(Console_ForegroundGreen | Console_Bold) << "Enabled features: " << ConsoleColorTag(Console_Normal)
		<< boost::algorithm::join(enabled_features, " ") << "\n";

	return 0;
}

bool FeatureUtility::GetFeatures(std::vector<String>& features, bool get_disabled)
{
	/* request all disabled features */
	if (get_disabled) {
		/* disable = available-enabled */
		String available_pattern = GetFeaturesAvailablePath() + "/*.conf";
		std::vector<String> available;
		Utility::Glob(available_pattern, [&available](const String& featureFile) { CollectFeatures(featureFile, available); }, GlobFile);

		String enabled_pattern = GetFeaturesEnabledPath() + "/*.conf";
		std::vector<String> enabled;
		Utility::Glob(enabled_pattern, [&enabled](const String& featureFile) { CollectFeatures(featureFile, enabled); }, GlobFile);

		std::sort(available.begin(), available.end());
		std::sort(enabled.begin(), enabled.end());
		std::set_difference(
			available.begin(), available.end(),
			enabled.begin(), enabled.end(),
			std::back_inserter(features)
		);
	} else {
		/* all enabled features */
		String enabled_pattern = GetFeaturesEnabledPath() + "/*.conf";

		Utility::Glob(enabled_pattern, [&features](const String& featureFile) { CollectFeatures(featureFile, features); }, GlobFile);
	}

	return true;
}

bool FeatureUtility::CheckFeatureEnabled(const String& feature)
{
	return CheckFeatureInternal(feature, false);
}

bool FeatureUtility::CheckFeatureDisabled(const String& feature)
{
	return CheckFeatureInternal(feature, true);
}

bool FeatureUtility::CheckFeatureInternal(const String& feature, bool check_disabled)
{
	std::vector<String> features;

	if (!FeatureUtility::GetFeatures(features, check_disabled))
		return false;

	for (const String& check_feature : features) {
		if (check_feature == feature)
			return true;
	}

	return false;
}

void FeatureUtility::CollectFeatures(const String& feature_file, std::vector<String>& features)
{
	String feature = Utility::BaseName(feature_file);
	boost::algorithm::replace_all(feature, ".conf", "");

	Log(LogDebug, "cli")
		<< "Adding feature: " << feature;
	features.push_back(feature);
}
