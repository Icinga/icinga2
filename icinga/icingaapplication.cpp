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
	/* register handler for 'log' config objects */
	static ConfigObject::Set::Ptr logObjects = boost::make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("log"));
	logObjects->OnObjectAdded.connect(boost::bind(&IcingaApplication::NewLogHandler, this, _2));
	logObjects->OnObjectCommitted.connect(boost::bind(&IcingaApplication::NewLogHandler, this, _2));
	logObjects->OnObjectRemoved.connect(boost::bind(&IcingaApplication::DeletedLogHandler, this, _2));
	logObjects->Start();

	/* create console logger */
	ConfigItemBuilder::Ptr consoleLogConfig = boost::make_shared<ConfigItemBuilder>();
	consoleLogConfig->SetType("log");
	consoleLogConfig->SetName("console");
	consoleLogConfig->SetLocal(true);
	consoleLogConfig->AddExpression("type", OperatorSet, "console");
	consoleLogConfig->Compile()->Commit();

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

	bool enableSyslog = false;
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
			if (arg == "-S") {
				enableSyslog = true;
				continue;
			} else if (arg == "-L") {
				if (it + 1 == args.end())
					throw invalid_argument("Option -L requires a parameter");

				StreamLogger::Ptr fileLogger = boost::make_shared<StreamLogger>(LogInformation);
				fileLogger->OpenFile(*(it + 1));
				Logger::RegisterLogger(fileLogger);

				it++;

				continue;
			} else if (arg == "-d") {
				daemonize = true;
				continue;
			} else {
				throw invalid_argument("Unknown option: " + arg);
			}
		}

		configFile = arg;

		if (it + 1 != args.end())
			throw invalid_argument("Trailing command line arguments after config filename.");
	}

	if (configFile.empty())
		throw invalid_argument("No config file was specified on the command line.");

	if (enableSyslog) {
#ifndef _WIN32
		SyslogLogger::Ptr syslogLogger = boost::make_shared<SyslogLogger>(LogInformation);
		Logger::RegisterLogger(syslogLogger);
#else /* _WIN32 */
		throw invalid_argument("Syslog is not supported on Windows.");
#endif /* _WIN32 */
	}

	string componentDirectory = Utility::DirName(GetExePath()) + "/../lib/icinga2";
	AddComponentSearchDir(componentDirectory);

	/* register handler for 'component' config objects */
	static ConfigObject::Set::Ptr componentObjects = boost::make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("component"));
	componentObjects->OnObjectAdded.connect(boost::bind(&IcingaApplication::NewComponentHandler, this, _2));
	componentObjects->OnObjectCommitted.connect(boost::bind(&IcingaApplication::NewComponentHandler, this, _2));
	componentObjects->OnObjectRemoved.connect(boost::bind(&IcingaApplication::DeletedComponentHandler, this, _2));
	componentObjects->Start();

	/* load convenience config component */
	ConfigItemBuilder::Ptr convenienceComponentConfig = boost::make_shared<ConfigItemBuilder>();
	convenienceComponentConfig->SetType("component");
	convenienceComponentConfig->SetName("convenience");
	convenienceComponentConfig->SetLocal(true);
	convenienceComponentConfig->Compile()->Commit();

	/* load config file */
	ConfigItemBuilder::Ptr fileComponentConfig = boost::make_shared<ConfigItemBuilder>();
	fileComponentConfig->SetType("component");
	fileComponentConfig->SetName("configfile");
	fileComponentConfig->SetLocal(true);
	fileComponentConfig->AddExpression("configFilename", OperatorSet, configFile);
	fileComponentConfig->Compile()->Commit();

	ConfigObject::Ptr icingaConfig = ConfigObject::GetObject("application", "icinga");

	if (!icingaConfig)
		throw runtime_error("Configuration must contain an 'application' object named 'icinga'.");

	if (!icingaConfig->IsLocal())
		throw runtime_error("'icinga' application object must be 'local'.");

	icingaConfig->GetProperty("cert", &m_CertificateFile);
	icingaConfig->GetProperty("ca", &m_CAFile);
	icingaConfig->GetProperty("node", &m_Node);
	icingaConfig->GetProperty("service", &m_Service);
	icingaConfig->GetProperty("pidpath", &m_PidPath);
	icingaConfig->GetProperty("macros", &m_Macros);

	string logpath;
	if (icingaConfig->GetProperty("logpath", &logpath)) {
		ConfigItemBuilder::Ptr fileLogConfig = boost::make_shared<ConfigItemBuilder>();
		fileLogConfig->SetType("log");
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

	return EXIT_SUCCESS;
}

void IcingaApplication::NewComponentHandler(const ConfigObject::Ptr& object)
{
	/* don't allow replicated config objects */
	if (!object->IsLocal())
		throw runtime_error("'component' objects must be 'local'");

	string path;
	if (!object->GetProperty("path", &path)) {
#ifdef _WIN32
		path = object->GetName() + ".dll";
#else /* _WIN32 */
		path = object->GetName() + ".la";
#endif /* _WIN32 */
	}

	LoadComponent(path, object);
}

void IcingaApplication::NewLogHandler(const ConfigObject::Ptr& object)
{
	/* don't allow replicated config objects */
	if (!object->IsLocal())
		throw runtime_error("'log' objects must be 'local'");

	Logger::Ptr logger;
	if (object->GetTag("logger", &logger))
		Logger::UnregisterLogger(logger);

	string type;
	if (!object->GetProperty("type", &type))
		throw invalid_argument("'log' object must have a 'type' property");

	string strSeverity;
	LogSeverity severity = LogInformation;
	if (object->GetProperty("severity", &strSeverity))
		severity = Logger::StringToSeverity(strSeverity);

	if (type == "syslog") {
		logger = boost::make_shared<SyslogLogger>(severity);
	} else if (type == "file") {
		string path;
		if (!object->GetProperty("path", &path))
			throw invalid_argument("'log' object of type 'file' must have a 'path' property");

		StreamLogger::Ptr slogger = boost::make_shared<StreamLogger>(severity);
		slogger->OpenFile(path);

		logger = slogger;
	} else if (type == "console") {
		logger = boost::make_shared<StreamLogger>(&std::cout, severity);
	} else {
		throw runtime_error("Unknown log type: " + type);
	}

	object->SetTag("logger", logger);

	Logger::RegisterLogger(logger);
}

void IcingaApplication::DeletedLogHandler(const ConfigObject::Ptr& object)
{
	Logger::Ptr logger;
	if (object->GetTag("logger", &logger))
		Logger::UnregisterLogger(logger);
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

void IcingaApplication::DeletedComponentHandler(const ConfigObject::Ptr& object)
{
	Component::Ptr component = GetComponent(object->GetName());
	UnregisterComponent(component);
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
