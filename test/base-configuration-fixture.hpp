/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef CONFIGURATION_FIXTURE_H
#define CONFIGURATION_FIXTURE_H

#include "base/configuration.hpp"
#include <boost/filesystem.hpp>
#include <BoostTestTargetConfig.h>

namespace icinga {

struct ConfigurationDataDirFixture
{
	ConfigurationDataDirFixture() : m_DataDir(boost::filesystem::current_path() / "data"), m_PrevDataDir(Configuration::DataDir.GetData())
	{
		Configuration::DataDir = m_DataDir.string();
		boost::filesystem::create_directories(m_DataDir);
	}

	~ConfigurationDataDirFixture()
	{
		boost::filesystem::remove_all(m_DataDir);
		Configuration::DataDir = m_PrevDataDir.string();
	}

	boost::filesystem::path m_DataDir;
	boost::filesystem::path m_PrevDataDir;
};

struct ConfigurationCacheDirFixture
{
	ConfigurationCacheDirFixture() : m_CacheDir(boost::filesystem::current_path() / "data"), m_PrevCacheDir(Configuration::CacheDir.GetData())
	{
		Configuration::CacheDir = m_CacheDir.string();
		boost::filesystem::create_directories(m_CacheDir);
	}

	~ConfigurationCacheDirFixture()
	{
		boost::filesystem::remove_all(m_CacheDir);
		Configuration::CacheDir = m_PrevCacheDir.string();
	}

	boost::filesystem::path m_CacheDir;
	boost::filesystem::path m_PrevCacheDir;
};

} // namespace icinga

#endif // CONFIGURATION_FIXTURE_H
