// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
		: m_DataDir(boost::filesystem::current_path() / "data" / std::string{Utility::NewUniqueID()}),
		  m_PrevDataDir(Configuration::DataDir.GetData())
	{
		boost::filesystem::create_directories(m_DataDir);
		Configuration::DataDir = m_DataDir.string();
	}

	~ConfigurationDataDirFixture()
	{
		boost::filesystem::remove_all(m_DataDir);
		Configuration::DataDir = m_PrevDataDir.string();
	}

private:
	boost::filesystem::path m_DataDir;
	boost::filesystem::path m_PrevDataDir;
};

struct ConfigurationCacheDirFixture
{
	ConfigurationCacheDirFixture()
		: m_CacheDir(boost::filesystem::current_path() / "cache" / std::string{Utility::NewUniqueID()}),
		  m_PrevCacheDir(Configuration::CacheDir.GetData())
	{
		boost::filesystem::create_directories(m_CacheDir);
		Configuration::CacheDir = m_CacheDir.string();
	}

	~ConfigurationCacheDirFixture()
	{
		boost::filesystem::remove_all(m_CacheDir);
		Configuration::CacheDir = m_PrevCacheDir.string();
	}

private:
	boost::filesystem::path m_CacheDir;
	boost::filesystem::path m_PrevCacheDir;
};

} // namespace icinga

#endif // CONFIGURATION_FIXTURE_H
