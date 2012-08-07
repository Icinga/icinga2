/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include <cstdio>
#include <iostream>
#include "i2-icinga.h"

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION GIT_MESSAGE
#endif /* _WIN32 */

using namespace icinga;

const String IcingaApplication::DefaultPidPath = "icinga.pid";
const String IcingaApplication::DefaultStatePath = "icinga.state";

IcingaApplication::IcingaApplication(void)
{ }

/**
 * The entry point for the Icinga application.
 *
 * @param args Command-line arguments.
 * @returns An exit status.
 */
int IcingaApplication::Main(const vector<String>& args)
{
	/* create console logger */
	ConfigItemBuilder::Ptr consoleLogConfig = boost::make_shared<ConfigItemBuilder>();
	consoleLogConfig->SetType("Logger");
	consoleLogConfig->SetName("console");
	consoleLogConfig->SetLocal(true);
	consoleLogConfig->AddExpression("type", OperatorSet, "console");
	consoleLogConfig->Compile()->Commit();
	consoleLogConfig.reset();

#ifdef _WIN32
	Logger::Write(LogInformation, "icinga", "Icinga component loader");
#else /* _WIN32 */
	Logger::Write(LogInformation, "icinga", "Icinga component loader (version: " ICINGA_VERSION ")");
#endif  /* _WIN32 */

	m_StartTime = Utility::GetTime();

	if (args.size() < 2) {
		stringstream msgbuf;
		msgbuf << "Syntax: " << args[0] << " [-S] [-L logfile] [-d] [--] <config-file>";
		Logger::Write(LogInformation, "icinga", msgbuf.str());
		return EXIT_FAILURE;
	}

	bool daemonize = false;
	bool parseOpts = true;
	String configFile;

	/* TODO: clean up this mess; for now it will just have to do */
	vector<String>::const_iterator it;
	for (it = args.begin() + 1 ; it != args.end(); it++) {
		String arg = *it;

		/* ignore empty arguments */
		if (arg.IsEmpty())
			continue;

		if (arg == "--") {
			parseOpts = false;
			continue;
		}

		if (parseOpts && arg[0] == '-') {
			if (arg == "-d") {
				daemonize = true;
				continue;
			} else {
				throw_exception(invalid_argument("Unknown option: " + arg));
			}
		}

		configFile = arg;

		if (it + 1 != args.end())
			throw_exception(invalid_argument("Trailing command line arguments after config filename."));
	}

	if (configFile.IsEmpty())
		throw_exception(invalid_argument("No config file was specified on the command line."));

	String componentDirectory = Utility::DirName(GetExePath()) + "/../lib/icinga2";
	Component::AddSearchDir(componentDirectory);

	/* load cibsync config component */
	ConfigItemBuilder::Ptr cibsyncComponentConfig = boost::make_shared<ConfigItemBuilder>();
	cibsyncComponentConfig->SetType("Component");
	cibsyncComponentConfig->SetName("cibsync");
	cibsyncComponentConfig->SetLocal(true);
	cibsyncComponentConfig->Compile()->Commit();
	cibsyncComponentConfig.reset();

	/* load convenience config component */
	ConfigItemBuilder::Ptr convenienceComponentConfig = boost::make_shared<ConfigItemBuilder>();
	convenienceComponentConfig->SetType("Component");
	convenienceComponentConfig->SetName("convenience");
	convenienceComponentConfig->SetLocal(true);
	convenienceComponentConfig->Compile()->Commit();
	convenienceComponentConfig.reset();

	/* load config file */
	vector<ConfigItem::Ptr> configItems = ConfigCompiler::CompileFile(configFile);

	Logger::Write(LogInformation, "icinga", "Executing config items...");

	BOOST_FOREACH(const ConfigItem::Ptr& item, configItems) {
		item->Commit();
	}

	DynamicObject::Ptr icingaConfig = DynamicObject::GetObject("Application", "icinga");

	if (!icingaConfig)
		throw_exception(runtime_error("Configuration must contain an 'Application' object named 'icinga'."));

	if (!icingaConfig->IsLocal())
		throw_exception(runtime_error("'icinga' application object must be 'local'."));

	m_CertificateFile = icingaConfig->Get("cert");
	m_CAFile = icingaConfig->Get("ca");
	m_Node = icingaConfig->Get("node");
	m_Service = icingaConfig->Get("service");

	m_PidPath = icingaConfig->Get("pidpath");
	if (m_PidPath.IsEmpty())
		m_PidPath = DefaultPidPath;

	m_StatePath = icingaConfig->Get("statepath");
	if (m_StatePath.IsEmpty())
		m_StatePath = DefaultStatePath;

	m_Macros = icingaConfig->Get("macros");

	String logpath = icingaConfig->Get("logpath");
	if (!logpath.IsEmpty()) {
		ConfigItemBuilder::Ptr fileLogConfig = boost::make_shared<ConfigItemBuilder>();
		fileLogConfig->SetType("Logger");
		fileLogConfig->SetName("main");
		fileLogConfig->SetLocal(true);
		fileLogConfig->AddExpression("type", OperatorSet, "file");
		fileLogConfig->AddExpression("path", OperatorSet, logpath);
		fileLogConfig->Compile()->Commit();
	}

	UpdatePidFile(GetPidPath());

	if (!GetCertificateFile().IsEmpty() && !GetCAFile().IsEmpty()) {
		/* set up SSL context */
		shared_ptr<X509> cert = Utility::GetX509Certificate(GetCertificateFile());
		String identity = Utility::GetCertificateCN(cert);
		Logger::Write(LogInformation, "icinga", "My identity: " + identity);
		EndpointManager::GetInstance()->SetIdentity(identity);

		shared_ptr<SSL_CTX> sslContext = Utility::MakeSSLContext(GetCertificateFile(), GetCertificateFile(), GetCAFile());
		EndpointManager::GetInstance()->SetSSLContext(sslContext);
	}

	/* create the primary RPC listener */
	String service = GetService();
	if (!service.IsEmpty())
		EndpointManager::GetInstance()->AddListener(service);

	if (daemonize) {
		Logger::Write(LogInformation, "icinga", "Daemonizing.");
		ClosePidFile();
		Utility::Daemonize();
		UpdatePidFile(GetPidPath());
	}

	/* restore the previous program state */
	DynamicObject::RestoreObjects(GetStatePath());

	/* periodically dump the program state */
	m_RetentionTimer = boost::make_shared<Timer>();
	m_RetentionTimer->SetInterval(300);
	m_RetentionTimer->OnTimerExpired.connect(boost::bind(&IcingaApplication::DumpProgramState, this));
	m_RetentionTimer->Start();

	RunEventLoop();

	DumpProgramState();

	Logger::Write(LogInformation, "icinga", "Icinga shutting down.");

	return EXIT_SUCCESS;
}

void IcingaApplication::DumpProgramState(void) {
	String temp = GetStatePath() + ".tmp";
	DynamicObject::DumpObjects(temp);
	rename(temp.CStr(), GetStatePath().CStr());
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

String IcingaApplication::GetCertificateFile(void) const
{
	return m_CertificateFile;
}

String IcingaApplication::GetCAFile(void) const
{
	return m_CAFile;
}

String IcingaApplication::GetNode(void) const
{
	return m_Node;
}

String IcingaApplication::GetService(void) const
{
	return m_Service;
}

String IcingaApplication::GetPidPath(void) const
{
	return m_PidPath;
}

String IcingaApplication::GetStatePath(void) const
{
	return m_StatePath;
}

Dictionary::Ptr IcingaApplication::GetMacros(void) const
{
	return m_Macros;
}

double IcingaApplication::GetStartTime(void) const
{
	return m_StartTime;
}
