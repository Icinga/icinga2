// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "base/i2-base.hpp"
#include "base/configuration-ti.hpp"

namespace icinga
{

/**
 * Global configuration.
 *
 * @ingroup base
 */
class Configuration : public ObjectImpl<Configuration>
{
public:
	DECLARE_OBJECT(Configuration);

	String GetApiBindHost() const override;
	void SetApiBindHost(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetApiBindPort() const override;
	void SetApiBindPort(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	bool GetAttachDebugger() const override;
	void SetAttachDebugger(bool value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetCacheDir() const override;
	void SetCacheDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	int GetConcurrency() const override;
	void SetConcurrency(int value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetConfigDir() const override;
	void SetConfigDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetDataDir() const override;
	void SetDataDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetEventEngine() const override;
	void SetEventEngine(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetIncludeConfDir() const override;
	void SetIncludeConfDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetInitRunDir() const override;
	void SetInitRunDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetLogDir() const override;
	void SetLogDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetModAttrPath() const override;
	void SetModAttrPath(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetObjectsPath() const override;
	void SetObjectsPath(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetPidPath() const override;
	void SetPidPath(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetPkgDataDir() const override;
	void SetPkgDataDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetPrefixDir() const override;
	void SetPrefixDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetProgramData() const override;
	void SetProgramData(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	int GetRLimitFiles() const override;
	void SetRLimitFiles(int value, bool suppress_events = false, const Value& cookie = Empty) override;

	int GetRLimitProcesses() const override;
	void SetRLimitProcesses(int value, bool suppress_events = false, const Value& cookie = Empty) override;

	int GetRLimitStack() const override;
	void SetRLimitStack(int value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetRunAsGroup() const override;
	void SetRunAsGroup(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetRunAsUser() const override;
	void SetRunAsUser(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetSpoolDir() const override;
	void SetSpoolDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetStatePath() const override;
	void SetStatePath(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	double GetTlsHandshakeTimeout() const override;
	void SetTlsHandshakeTimeout(double value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetVarsPath() const override;
	void SetVarsPath(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetZonesDir() const override;
	void SetZonesDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	/* deprecated */
	String GetLocalStateDir() const override;
	void SetLocalStateDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetSysconfDir() const override;
	void SetSysconfDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	String GetRunDir() const override;
	void SetRunDir(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	static bool GetReadOnly();
	static void SetReadOnly(bool readOnly);

	static String ApiBindHost;
	static String ApiBindPort;
	static bool AttachDebugger;
	static String CacheDir;
	static int Concurrency;
	static bool ConcurrencyWasModified;
	static String ConfigDir;
	static String DataDir;
	static String EventEngine;
	static String IncludeConfDir;
	static String InitRunDir;
	static String LogDir;
	static String ModAttrPath;
	static String ObjectsPath;
	static String PidPath;
	static String PkgDataDir;
	static String PrefixDir;
	static String ProgramData;
	static int RLimitFiles;
	static int RLimitProcesses;
	static int RLimitStack;
	static String RunAsGroup;
	static String RunAsUser;
	static String SpoolDir;
	static String StatePath;
	static double TlsHandshakeTimeout;
	static String VarsPath;
	static String ZonesDir;

	/* deprecated */
	static String LocalStateDir;
	static String RunDir;
	static String SysconfDir;

private:
	static bool m_ReadOnly;

};

}

#endif /* CONFIGURATION_H */
