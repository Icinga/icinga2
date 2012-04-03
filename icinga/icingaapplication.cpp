#include <cstdio>
#include <iostream>
#include "i2-icinga.h"

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION GIT_MESSAGE
#endif /* _WIN32 */

using namespace icinga;

IcingaApplication::IcingaApplication(void)
{
	m_ConnectionManager = make_shared<ConnectionManager>();
}

int IcingaApplication::Main(const vector<string>& args)
{
#ifdef _WIN32
	cout << "Icinga component loader" << endl;
#else /* _WIN32 */
	cout << "Icinga component loader (version: " << ICINGA_VERSION << ")" << endl;
#endif  /* _WIN32 */

	if (args.size() < 2) {
		PrintUsage(args[0]);
		return EXIT_FAILURE;
	}

	string componentDirectory = GetExeDirectory() + "/../lib/icinga";
	AddComponentSearchDir(componentDirectory);

	GetConfigHive()->OnObjectCreated += bind_weak(&IcingaApplication::ConfigObjectCreatedHandler, shared_from_this());
	GetConfigHive()->OnObjectRemoved += bind_weak(&IcingaApplication::ConfigObjectRemovedHandler, shared_from_this());

	ConfigObject::Ptr fileComponentConfig = make_shared<ConfigObject>();
	fileComponentConfig->SetName("configfilecomponent");
	fileComponentConfig->SetType("component");
	fileComponentConfig->SetProperty("configFilename", args[1]);
	GetConfigHive()->AddObject(fileComponentConfig);

	RunEventLoop();

	return EXIT_SUCCESS;
}

void IcingaApplication::PrintUsage(const string& programPath)
{
	cout << "Syntax: " << programPath << " <config-file>" << endl;
}

ConnectionManager::Ptr IcingaApplication::GetConnectionManager(void)
{
	return m_ConnectionManager;
}

int IcingaApplication::ConfigObjectCreatedHandler(ConfigHiveEventArgs::Ptr ea)
{
	if (ea->Object->GetType() == "component") {
		string path;
		
		if (!ea->Object->GetProperty("path", &path)) {
#ifdef _WIN32
			path = ea->Object->GetName() + ".dll";
#else /* _WIN32 */
			path = "lib" + ea->Object->GetName() + ".la";
#endif /* _WIN32 */

			// TODO: try to figure out where the component is located */
		}

		LoadComponent(path, ea->Object);
	}

	return 0;
}

int IcingaApplication::ConfigObjectRemovedHandler(ConfigHiveEventArgs::Ptr ea)
{
	if (ea->Object->GetType() == "component") {
		UnloadComponent(ea->Object->GetName());
	}

	return 0;
}

SET_START_CLASS(icinga::IcingaApplication);
