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
	ConfigObject::RefType fileComponentConfig = new_object<ConfigObject>();
	fileComponentConfig->SetName("configfilecomponent");
	fileComponentConfig->SetType("component");
	fileComponentConfig->SetProperty("filename", "icinga.conf");
	GetConfigHive()->AddObject(fileComponentConfig);

	LoadComponent("configfilecomponent");

	LoadComponent("configrpccomponent");

	RunEventLoop();

	return 0;
}

ConnectionManager::RefType IcingaApplication::GetConnectionManager(void)
{
	return m_ConnectionManager;
}

SET_START_CLASS(icinga::IcingaApplication);
