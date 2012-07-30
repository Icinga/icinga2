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

const string IcingaApplication::DefaultPidPath = "icinga.pid";

IcingaApplication::IcingaApplication(void)
	: m_PidPath(DefaultPidPath)
{ }

/**
 * The entry point for the Icinga application.
 *
 * @param args Command-line arguments.
 * @returns An exit status.
 */
int IcingaApplication::Main(const vector<string>& args)
{
	/* create console logger */
	ConfigItemBuilder::Ptr consoleLogConfig = boost::make_shared<ConfigItemBuilder>();
	consoleLogConfig->SetType("Logger");
	consoleLogConfig->SetName("console");
	consoleLogConfig->SetLocal(true);
	consoleLogConfig->AddExpression("type", OperatorSet, "console");
	consoleLogConfig->Compile()->Commit();

	/* restore the previous program state */
	DynamicObject::RestoreObjects("retention.dat");

	/* periodically dump the program state */
	m_RetentionTimer = boost::make_shared<Timer>();
	m_RetentionTimer->SetInterval(60);
	m_RetentionTimer->OnTimerExpired.connect(boost::bind(&IcingaApplication::DumpProgramState, this));
	m_RetentionTimer->Start();

#ifdef _WIN32
	Logger::Write(LogInformation, "icinga", "Icinga component loader");
#else /* _WIN32 */
	Logger::Write(LogInformation, "icinga", "Icinga component loader (version: " ICINGA_VERSION ")");
#endif  /* _WIN32 */

	time(&m_StartTime);

	if (args.size() < 2) {
		stringstream msgbuf;
		msgbuf << "Syntax: " << args[0] << " [-S] [-L logfile] [-d] [--] <config-file>";
		Logger::Write(LogInformation, "icinga", msgbuf.str());
		return EXIT_FAILURE;
	}

	bool daemonize = false;
	bool parseOpts = true;
	string configFile;

	/* TODO: clean up this mess; for now it will just have to do */
	vector<string>::const_iterator it;
	for (it = args.begin() + 1 ; it != args.end(); it++) {
		string arg = *it;

		/* ignore empty arguments */
		if (arg.empty())
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

	if (configFile.empty())
		throw_exception(invalid_argument("No config file was specified on the command line."));

	string componentDirectory = Utility::DirName(GetExePath()) + "/../lib/icinga2";
	Component::AddSearchDir(componentDirectory);

	/* load cibsync config component */
	ConfigItemBuilder::Ptr cibsyncComponentConfig = boost::make_shared<ConfigItemBuilder>();
	cibsyncComponentConfig->SetType("Component");
	cibsyncComponentConfig->SetName("cibsync");
	cibsyncComponentConfig->SetLocal(true);
	cibsyncComponentConfig->Compile()->Commit();

	/* load convenience config component */
	ConfigItemBuilder::Ptr convenienceComponentConfig = boost::make_shared<ConfigItemBuilder>();
	convenienceComponentConfig->SetType("Component");
	convenienceComponentConfig->SetName("convenience");
	convenienceComponentConfig->SetLocal(true);
	convenienceComponentConfig->Compile()->Commit();

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

	icingaConfig->GetProperty("cert", &m_CertificateFile);
	icingaConfig->GetProperty("ca", &m_CAFile);
	icingaConfig->GetProperty("node", &m_Node);
	icingaConfig->GetProperty("service", &m_Service);
	icingaConfig->GetProperty("pidpath", &m_PidPath);
	icingaConfig->GetProperty("macros", &m_Macros);

	string logpath;
	if (icingaConfig->GetProperty("logpath", &logpath)) {
		ConfigItemBuilder::Ptr fileLogConfig = boost::make_shared<ConfigItemBuilder>();
		fileLogConfig->SetType("Logger");
		fileLogConfig->SetName("main");
		fileLogConfig->SetLocal(true);
		fileLogConfig->AddExpression("type", OperatorSet, "file");
		fileLogConfig->AddExpression("path", OperatorSet, logpath);
		fileLogConfig->Compile()->Commit();
	}

	UpdatePidFile(GetPidPath());

	if (!GetCertificateFile().empty() && !GetCAFile().empty()) {
		/* set up SSL context */
		shared_ptr<X509> cert = Utility::GetX509Certificate(GetCertificateFile());
		string identity = Utility::GetCertificateCN(cert);
		Logger::Write(LogInformation, "icinga", "My identity: " + identity);
		EndpointManager::GetInstance()->SetIdentity(identity);

		shared_ptr<SSL_CTX> sslContext = Utility::MakeSSLContext(GetCertificateFile(), GetCertificateFile(), GetCAFile());
		EndpointManager::GetInstance()->SetSSLContext(sslContext);
	}

	/* create the primary RPC listener */
	string service = GetService();
	if (!service.empty())
		EndpointManager::GetInstance()->AddListener(service);

	if (daemonize) {
		Logger::Write(LogInformation, "icinga", "Daemonizing.");
		ClosePidFile();
		Utility::Daemonize();
		UpdatePidFile(GetPidPath());
	}

	RunEventLoop();

	DumpProgramState();

	Logger::Write(LogInformation, "icinga", "Icinga shutting down.");

	return EXIT_SUCCESS;
}

void IcingaApplication::DumpProgramState(void) {
	DynamicObject::DumpObjects("retention.dat.tmp");
	rename("retention.dat.tmp", "retention.dat");
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

string IcingaApplication::GetCertificateFile(void) const
{
	return m_CertificateFile;
}

string IcingaApplication::GetCAFile(void) const
{
	return m_CAFile;
}

string IcingaApplication::GetNode(void) const
{
	return m_Node;
}

string IcingaApplication::GetService(void) const
{
	return m_Service;
}

string IcingaApplication::GetPidPath(void) const
{
	return m_PidPath;
}

Dictionary::Ptr IcingaApplication::GetMacros(void) const
{
	return m_Macros;
}

time_t IcingaApplication::GetStartTime(void) const
{
	return m_StartTime;
}
