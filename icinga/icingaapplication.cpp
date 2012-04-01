#include <cstdio>
#include <iostream>
#include "i2-icinga.h"

using namespace icinga;

using std::cout;
using std::endl;

IcingaApplication::IcingaApplication(void)
{
	m_ConnectionManager = new_object<ConnectionManager>();
}

int IcingaApplication::Main(const vector<string>& args)
{
	GetConfigHive()->OnObjectCreated.bind(bind_weak(&IcingaApplication::ConfigObjectCreatedHandler, shared_from_this()));
	GetConfigHive()->OnObjectRemoved.bind(bind_weak(&IcingaApplication::ConfigObjectRemovedHandler, shared_from_this()));

	ConfigObject::RefType fileComponentConfig = new_object<ConfigObject>();
	fileComponentConfig->SetName("configfilecomponent");
	fileComponentConfig->SetType("component");
	fileComponentConfig->SetProperty("filename", "icinga.conf");
	GetConfigHive()->AddObject(fileComponentConfig);

	RunEventLoop();

	return 0;
}

ConnectionManager::RefType IcingaApplication::GetConnectionManager(void)
{
	return m_ConnectionManager;
}

int IcingaApplication::ConfigObjectCreatedHandler(ConfigHiveEventArgs::RefType ea)
{
	if (ea->Object->GetType() == "component") {
		LoadComponent(ea->Object->GetName());
	}

	return 0;
}

int IcingaApplication::ConfigObjectRemovedHandler(ConfigHiveEventArgs::RefType ea)
{
	if (ea->Object->GetType() == "component") {
		UnloadComponent(ea->Object->GetName());
	}

	return 0;
}

SET_START_CLASS(icinga::IcingaApplication);
