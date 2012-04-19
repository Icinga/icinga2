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

	int configSource;
	if (GetConfig()->GetPropertyInteger("configSource", &configSource) && configSource != 0) {
		m_ConfigRpcEndpoint->RegisterMethodHandler("config::FetchObjects", bind_weak(&ConfigRpcComponent::FetchObjectsHandler, shared_from_this()));

		configHive->OnObjectCreated += bind_weak(&ConfigRpcComponent::LocalObjectCreatedHandler, shared_from_this());
		configHive->OnObjectRemoved += bind_weak(&ConfigRpcComponent::LocalObjectRemovedHandler, shared_from_this());
		configHive->OnPropertyChanged += bind_weak(&ConfigRpcComponent::LocalPropertyChangedHandler, shared_from_this());

		m_ConfigRpcEndpoint->RegisterMethodSource("config::ObjectCreated");
		m_ConfigRpcEndpoint->RegisterMethodSource("config::ObjectRemoved");
		m_ConfigRpcEndpoint->RegisterMethodSource("config::PropertyChanged");
	}

	m_ConfigRpcEndpoint->RegisterMethodHandler("config::ObjectCreated", bind_weak(&ConfigRpcComponent::RemoteObjectUpdatedHandler, shared_from_this()));
	m_ConfigRpcEndpoint->RegisterMethodHandler("config::ObjectRemoved", bind_weak(&ConfigRpcComponent::RemoteObjectRemovedHandler, shared_from_this()));
	m_ConfigRpcEndpoint->RegisterMethodHandler("config::PropertyChanged", bind_weak(&ConfigRpcComponent::RemoteObjectUpdatedHandler, shared_from_this()));

	endpointManager->RegisterEndpoint(m_ConfigRpcEndpoint);
}

void ConfigRpcComponent::Stop(void)
{
	// TODO: implement
}

JsonRpcRequest ConfigRpcComponent::MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties)
{
	JsonRpcRequest msg;
	msg.SetVersion("2.0");
	msg.SetMethod(method);

	Message params;
	msg.SetParams(params);

	params.GetDictionary()->SetValueString("name", object->GetName());
	params.GetDictionary()->SetValueString("type", object->GetType());

	if (includeProperties) {
		Message properties;
		params.GetDictionary()->SetValueDictionary("properties", properties.GetDictionary());

		for (ConfigObject::ParameterIterator pi = object->Properties.begin(); pi != object->Properties.end(); pi++) {
			properties.GetDictionary()->SetValueString(pi->first, pi->second);
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

int ConfigRpcComponent::LocalObjectCreatedHandler(const ConfigObjectEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	int replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMulticastRequest(m_ConfigRpcEndpoint, MakeObjectMessage(object, "config::ObjectCreated", true));
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(const ConfigObjectEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	int replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMulticastRequest(m_ConfigRpcEndpoint, MakeObjectMessage(object, "config::ObjectRemoved", false));
	}

	return 0;
}

int ConfigRpcComponent::LocalPropertyChangedHandler(const ConfigObjectEventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	int replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		JsonRpcRequest msg = MakeObjectMessage(object, "config::PropertyChanged", false);
		Message params;
		msg.SetParams(params);

		Message properties;
		params.GetDictionary()->SetValueDictionary("properties", properties.GetDictionary());

		string value;
		object->GetProperty(ea.Property, &value);

		properties.GetDictionary()->SetValueString(ea.Property, value);

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
	if (!params.GetDictionary()->GetValueString("name", &name))
		return 0;

	string type;
	if (!params.GetDictionary()->GetValueString("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object) {
		was_null = true;
		object = make_shared<ConfigObject>(type, name);
	}

	Dictionary::Ptr properties;
	if (!params.GetDictionary()->GetValueDictionary("properties", &properties))
		return 0;

	for (DictionaryIterator i = properties->Begin(); i != properties->End(); i++) {
		object->SetProperty(i->first, i->second);
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
	if (!params.GetDictionary()->GetValueString("name", &name))
		return 0;

	string type;
	if (!params.GetDictionary()->GetValueString("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object)
		return 0;

	configHive->RemoveObject(object);

	return 0;
}

EXPORT_COMPONENT(ConfigRpcComponent);
