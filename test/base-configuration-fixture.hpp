/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef CONFIGURATION_FIXTURE_H
#define CONFIGURATION_FIXTURE_H

#include "base/configuration.hpp"
#include "base/utility.hpp"
#include <boost/filesystem.hpp>
#include <BoostTestTargetConfig.h>

namespace icinga {

struct ConfigurationDataDirFixture
{
	ConfigurationDataDirFixture()
	{
		Utility::MkDir(tmpDir, 0700);
	}
	
	String tmpDir = boost::filesystem::temp_directory_path().append("icinga2").string();
	String previousDataDir = std::exchange(Configuration::DataDir, tmpDir);
};

struct ConfigurationDataDirCleanupFixture: ConfigurationDataDirFixture
{
	~ConfigurationDataDirCleanupFixture()
	{
		Utility::RemoveDirRecursive(std::exchange(Configuration::DataDir, previousDataDir));
	}
};

struct ConfigurationCacheDirFixture
{
	ConfigurationCacheDirFixture()
	{
		Utility::MkDir(tmpCacheDir, 0700);
	}
	
	String tmpCacheDir = boost::filesystem::temp_directory_path().append("icinga2").append("cache").string();
	String previousCacheDir = std::exchange(Configuration::CacheDir, tmpCacheDir);
};

struct ConfigurationCacheDirCleanupFixture: ConfigurationCacheDirFixture
{
	~ConfigurationCacheDirCleanupFixture()
	{
		Utility::RemoveDirRecursive(std::exchange(Configuration::CacheDir, previousCacheDir));
	}
};

} // namespace icinga

#endif // CONFIGURATION_FIXTURE_H
