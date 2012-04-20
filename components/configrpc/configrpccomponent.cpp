#include "i2-configrpc.h"

using namespace icinga;

IcingaApplication::Ptr ConfigRpcComponent::GetIcingaApplication(void)
{
	return static_pointer_cast<IcingaApplication>(GetApplication());
}

string ConfigRpcComponent::GetName(void) const
{
	return "configcomponent";
}

void ConfigRpcComponent::Start(void)
{
	IcingaApplication::Ptr icingaApp = GetIcingaApplication();

	EndpointManager::Ptr endpointManager = icingaApp->GetEndpointManager();
	ConfigHive::Ptr configHive = icingaApp->GetConfigHive();

	m_ConfigRpcEndpoint = make_shared<VirtualEndpoint>();

	long configSource;
	if (GetConfig()->GetPropertyInteger("configSource", &configSource) && configSource != 0) {
		m_ConfigRpcEndpoint->RegisterMethodHandler("config::FetchObjects", bind_weak(&ConfigRpcComponent::FetchObjectsHandler, shared_from_this()));

		configHive->OnObjectCreated += bind_weak(&ConfigRpcComponent::LocalObjectCreatedHandler, shared_from_this());
		configHive->OnObjectRemoved += bind_weak(&ConfigRpcComponent::LocalObjectRemovedHandler, shared_from_this());
		configHive->OnPropertyChanged += bind_weak(&ConfigRpcComponent::LocalPropertyChangedHandler, shared_from_this());

		m_ConfigRpcEndpoint->RegisterMethodSource("config::ObjectCreated");
		m_ConfigRpcEndpoint->RegisterMethodSource("config::ObjectRemoved");
		m_ConfigRpcEndpoint->RegisterMethodSource("config::PropertyChanged");
	}

	m_ConfigRpcEndpoint->RegisterMethodHandler("message::Welcome", bind_weak(&ConfigRpcComponent::WelcomeMessageHandler, shared_from_this()));

	m_ConfigRpcEndpoint->RegisterMethodHandler("config::ObjectCreated", bind_weak(&ConfigRpcComponent::RemoteObjectUpdatedHandler, shared_from_this()));
	m_ConfigRpcEndpoint->RegisterMethodHandler("config::ObjectRemoved", bind_weak(&ConfigRpcComponent::RemoteObjectRemovedHandler, shared_from_this()));
	m_ConfigRpcEndpoint->RegisterMethodHandler("config::PropertyChanged", bind_weak(&ConfigRpcComponent::RemoteObjectUpdatedHandler, shared_from_this()));

	endpointManager->RegisterEndpoint(m_ConfigRpcEndpoint);

	endpointManager->OnNewEndpoint += bind_weak(&ConfigRpcComponent::NewEndpointHandler, shared_from_this());
	endpointManager->ForeachEndpoint(bind(&ConfigRpcComponent::NewEndpointHandler, this, _1));
}

void ConfigRpcComponent::Stop(void)
{
	// TODO: implement
}

int ConfigRpcComponent::NewEndpointHandler(const NewEndpointEventArgs& ea)
{
	if (ea.Endpoint->HasIdentity()) {
		JsonRpcRequest request;
		request.SetVersion("2.0");
		request.SetMethod("config::FetchObjects");
		ea.Endpoint->ProcessRequest(m_ConfigRpcEndpoint, request);
	}

	return 0;
}

int ConfigRpcComponent::WelcomeMessageHandler(const NewRequestEventArgs& ea)
{
	NewEndpointEventArgs neea;
	neea.Source = shared_from_this();
	neea.Endpoint = ea.Sender;
	NewEndpointHandler(neea);

	return 0;
}

JsonRpcRequest ConfigRpcComponent::MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties)
{
	JsonRpcRequest msg;
	msg.SetVersion("2.0");
	msg.SetMethod(method);

	Message params;
	msg.SetParams(params);

	params.GetDictionary()->SetPropertyString("name", object->GetName());
	params.GetDictionary()->SetPropertyString("type", object->GetType());

	if (includeProperties) {
		Message properties;
		params.GetDictionary()->SetPropertyDictionary("properties", properties.GetDictionary());

		for (ConfigObject::ParameterIterator pi = object->Properties.begin(); pi != object->Properties.end(); pi++) {
			properties.GetDictionary()->SetPropertyString(pi->first, pi->second);
		}
	}

	return msg;
}

int ConfigRpcComponent::FetchObjectsHandler(const NewRequestEventArgs& ea)
{
	Endpoint::Ptr client = ea.Sender;
	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();

	for (ConfigHive::CollectionIterator ci = configHive->Collections.begin(); ci != configHive->Collections.end(); ci++) {
		ConfigCollection::Ptr collection = ci->second;

		for (ConfigCollection::ObjectIterator oi = collection->Objects.begin(); oi != collection->Objects.end(); oi++) {
			client->ProcessRequest(m_ConfigRpcEndpoint, MakeObjectMessage(oi->second, "config::ObjectCreated", true));
		}
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectCreatedHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	long replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMulticastRequest(m_ConfigRpcEndpoint, MakeObjectMessage(object, "config::ObjectCreated", true));
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	long replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMulticastRequest(m_ConfigRpcEndpoint, MakeObjectMessage(object, "config::ObjectRemoved", false));
	}

	return 0;
}

int ConfigRpcComponent::LocalPropertyChangedHandler(const DictionaryPropertyChangedEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	long replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		JsonRpcRequest msg = MakeObjectMessage(object, "config::PropertyChanged", false);
		Message params;
		msg.SetParams(params);

		Message properties;
		params.GetDictionary()->SetPropertyDictionary("properties", properties.GetDictionary());

		string value;
		if (!object->GetPropertyString(ea.Property, &value))
			return 0;

		properties.GetDictionary()->SetPropertyString(ea.Property, value);

		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMulticastRequest(m_ConfigRpcEndpoint, msg);
	}

	return 0;
}

int ConfigRpcComponent::RemoteObjectUpdatedHandler(const NewRequestEventArgs& ea)
{
	JsonRpcRequest message = ea.Request;
	bool was_null = false;

	Message params;
	if (!message.GetParams(&params))
		return 0;

	string name;
	if (!params.GetDictionary()->GetPropertyString("name", &name))
		return 0;

	string type;
	if (!params.GetDictionary()->GetPropertyString("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object) {
		was_null = true;
		object = make_shared<ConfigObject>(type, name);
	}

	Dictionary::Ptr properties;
	if (!params.GetDictionary()->GetPropertyDictionary("properties", &properties))
		return 0;

	for (DictionaryIterator i = properties->Begin(); i != properties->End(); i++) {
		object->SetPropertyString(i->first, i->second);
	}

	if (was_null)
		configHive->AddObject(object);

	return 0;
}

int ConfigRpcComponent::RemoteObjectRemovedHandler(const NewRequestEventArgs& ea)
{
	JsonRpcRequest message = ea.Request;
	
	Message params;
	if (!message.GetParams(&params))
		return 0;

	string name;
	if (!params.GetDictionary()->GetPropertyString("name", &name))
		return 0;

	string type;
	if (!params.GetDictionary()->GetPropertyString("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object)
		return 0;

	configHive->RemoveObject(object);

	return 0;
}

EXPORT_COMPONENT(ConfigRpcComponent);
