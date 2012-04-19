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
	m_EndpointManager = make_shared<EndpointManager>();
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

	ConfigCollection::Ptr componentCollection = GetConfigHive()->GetCollection("component");

	function<int (const ConfigObjectEventArgs&)> NewComponentHandler = bind_weak(&IcingaApplication::NewComponentHandler, shared_from_this());
	componentCollection->OnObjectCreated += NewComponentHandler;
	componentCollection->ForEachObject(NewComponentHandler);

	componentCollection->OnObjectRemoved += bind_weak(&IcingaApplication::DeletedComponentHandler, shared_from_this());

	ConfigCollection::Ptr listenerCollection = GetConfigHive()->GetCollection("rpclistener");

	function<int (const ConfigObjectEventArgs&)> NewRpcListenerHandler = bind_weak(&IcingaApplication::NewRpcListenerHandler, shared_from_this());
	listenerCollection->OnObjectCreated += NewRpcListenerHandler;
	listenerCollection->ForEachObject(NewRpcListenerHandler);

	listenerCollection->OnObjectRemoved += bind_weak(&IcingaApplication::DeletedRpcListenerHandler, shared_from_this());

	ConfigCollection::Ptr connectionCollection = GetConfigHive()->GetCollection("rpcconnection");

	function<int (const ConfigObjectEventArgs&)> NewRpcConnectionHandler = bind_weak(&IcingaApplication::NewRpcConnectionHandler, shared_from_this());
	connectionCollection->OnObjectCreated += NewRpcConnectionHandler;
	connectionCollection->ForEachObject(NewRpcConnectionHandler);

	connectionCollection->OnObjectRemoved += bind_weak(&IcingaApplication::DeletedRpcConnectionHandler, shared_from_this());

	ConfigObject::Ptr fileComponentConfig = make_shared<ConfigObject>("component", "configfile");
	fileComponentConfig->SetProperty("configFilename", args[1]);
	fileComponentConfig->SetPropertyInteger("replicate", 0);
	GetConfigHive()->AddObject(fileComponentConfig);

	ConfigCollection::Ptr collection = GetConfigHive()->GetCollection("rpclistener");

	m_TestEndpoint = make_shared<VirtualEndpoint>();
	m_EndpointManager->RegisterEndpoint(m_TestEndpoint);
	m_TestEndpoint->RegisterMethodSink("test");
	m_TestEndpoint->RegisterMethodSource("test");

	m_TestTimer = make_shared<Timer>();
	m_TestTimer->SetInterval(5);
	m_TestTimer->OnTimerExpired += bind_weak(&IcingaApplication::TestTimerHandler, shared_from_this());
	m_TestTimer->Start();

	RunEventLoop();

	return EXIT_SUCCESS;
}

int IcingaApplication::TestTimerHandler(const TimerEventArgs& tea)
{
	cout << "Problem?" << endl;

	JsonRpcRequest request;
	request.SetVersion("2.0");
	request.SetMethod("test");

	for (int i = 0; i < 1000; i++)
		m_EndpointManager->SendMulticastRequest(m_TestEndpoint, request);

	return 0;
}

void IcingaApplication::PrintUsage(const string& programPath)
{
	cout << "Syntax: " << programPath << " <config-file>" << endl;
}

EndpointManager::Ptr IcingaApplication::GetEndpointManager(void)
{
	return m_EndpointManager;
}

int IcingaApplication::NewComponentHandler(const ConfigObjectEventArgs& ea)
{
	string path;
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
		
	if (!object->GetProperty("path", &path)) {
#ifdef _WIN32
		path = object->GetName() + ".dll";
#else /* _WIN32 */
		path = object->GetName() + ".la";
#endif /* _WIN32 */

		// TODO: try to figure out where the component is located */
	}

	LoadComponent(path, object);

	return 0;
}

int IcingaApplication::DeletedComponentHandler(const ConfigObjectEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	Component::Ptr component = GetComponent(object->GetName());
	UnregisterComponent(component);

	return 0;
}

int IcingaApplication::NewRpcListenerHandler(const ConfigObjectEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	int port;

	if (!object->GetPropertyInteger("port", &port))
		throw Exception("Parameter 'port' is required for 'rpclistener' objects.");

	Log("Creating JSON-RPC listener on port %d", port);

	GetEndpointManager()->AddListener(port);

	return 0;
}

int IcingaApplication::DeletedRpcListenerHandler(const ConfigObjectEventArgs& ea)
{
	throw Exception("Unsupported operation.");

	return 0;
}

int IcingaApplication::NewRpcConnectionHandler(const ConfigObjectEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	string hostname;
	int port;

	if (!object->GetProperty("hostname", &hostname))
		throw Exception("Parameter 'hostname' is required for 'rpcconnection' objects.");

	if (!object->GetPropertyInteger("port", &port))
		throw Exception("Parameter 'port' is required for 'rpcconnection' objects.");

	Log("Creating JSON-RPC connection to %s:%d", hostname.c_str(), port);

	GetEndpointManager()->AddConnection(hostname, port);

	return 0;
}

int IcingaApplication::DeletedRpcConnectionHandler(const ConfigObjectEventArgs& ea)
{
	throw Exception("Unsupported operation.");

	return 0;
}
