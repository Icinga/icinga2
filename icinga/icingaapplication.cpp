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

	function<int (ConfigObjectEventArgs::Ptr)> NewComponentHandler;
	NewComponentHandler = bind_weak(&IcingaApplication::NewComponentHandler, shared_from_this());
	ConfigCollection::Ptr componentCollection = GetConfigHive()->GetCollection("component");
	componentCollection->OnObjectCreated += NewComponentHandler;
	componentCollection->ForEachObject(NewComponentHandler);

	function<int (ConfigObjectEventArgs::Ptr)> DeletedComponentHandler;
	DeletedComponentHandler = bind_weak(&IcingaApplication::DeletedComponentHandler, shared_from_this());
	componentCollection->OnObjectRemoved += DeletedComponentHandler;

	function<int (ConfigObjectEventArgs::Ptr)> NewRpcListenerHandler;
	NewRpcListenerHandler = bind_weak(&IcingaApplication::NewRpcListenerHandler, shared_from_this());
	ConfigCollection::Ptr listenerCollection = GetConfigHive()->GetCollection("rpclistener");
	listenerCollection->OnObjectCreated += NewRpcListenerHandler;
	listenerCollection->ForEachObject(NewRpcListenerHandler);

	function<int (ConfigObjectEventArgs::Ptr)> DeletedRpcListenerHandler;
	DeletedRpcListenerHandler = bind_weak(&IcingaApplication::DeletedRpcListenerHandler, shared_from_this());
	listenerCollection->OnObjectRemoved += DeletedRpcListenerHandler;

	ConfigObject::Ptr fileComponentConfig = make_shared<ConfigObject>("component", "configfilecomponent");
	fileComponentConfig->SetProperty("configFilename", args[1]);
	GetConfigHive()->AddObject(fileComponentConfig);

	ConfigCollection::Ptr collection = GetConfigHive()->GetCollection("rpclistener");

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

int IcingaApplication::NewComponentHandler(ConfigObjectEventArgs::Ptr ea)
{
	string path;
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea->Source);
		
	if (!object->GetProperty("path", &path)) {
#ifdef _WIN32
		path = object->GetName() + ".dll";
#else /* _WIN32 */
		path = "lib" + ea->Object->GetName() + ".la";
#endif /* _WIN32 */

		// TODO: try to figure out where the component is located */
	}

	LoadComponent(path, object);

	return 0;
}

int IcingaApplication::DeletedComponentHandler(ConfigObjectEventArgs::Ptr ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea->Source);
	UnloadComponent(object->GetName());

	return 0;
}

int IcingaApplication::NewRpcListenerHandler(ConfigObjectEventArgs::Ptr ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea->Source);
	int port;

	if (!object->GetPropertyInteger("port", &port))
		throw Exception("Parameter 'port' is required for 'rpclistener' objects.");

	Log("Creating JSON-RPC listener on port %d", port);

	JsonRpcServer::Ptr server = make_shared<JsonRpcServer>();
	server->Bind(port);
	server->Start();
	GetConnectionManager()->RegisterServer(server);

	return 0;
}

int IcingaApplication::DeletedRpcListenerHandler(ConfigObjectEventArgs::Ptr ea)
{
	throw Exception("Unsupported operation.");

	return 0;
}

SET_START_CLASS(icinga::IcingaApplication);
