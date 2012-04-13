#include "i2-configrpc.h"

using namespace icinga;

IcingaApplication::Ptr ConfigRpcComponent::GetIcingaApplication(void)
{
	return static_pointer_cast<IcingaApplication>(GetApplication());
}

string ConfigRpcComponent::GetName(void)
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

JsonRpcMessage::Ptr ConfigRpcComponent::MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties)
{
	JsonRpcMessage::Ptr msg = make_shared<JsonRpcMessage>();
	msg->SetVersion("2.0");
	msg->SetMethod(method);
	cJSON *params = msg->GetParams();

	string name = object->GetName();
	cJSON_AddStringToObject(params, "name", name.c_str());

	string type = object->GetType();
	cJSON_AddStringToObject(params, "type", type.c_str());

	if (includeProperties) {
		cJSON *properties = cJSON_CreateObject();
		cJSON_AddItemToObject(params, "properties", properties);

		for (ConfigObject::ParameterIterator pi = object->Properties.begin(); pi != object->Properties.end(); pi++) {
			cJSON_AddStringToObject(properties, pi->first.c_str(), pi->second.c_str());
		}
	}

	return msg;
}

int ConfigRpcComponent::FetchObjectsHandler(NewMessageEventArgs::Ptr ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea->Source);
	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();

	for (ConfigHive::CollectionIterator ci = configHive->Collections.begin(); ci != configHive->Collections.end(); ci++) {
		ConfigCollection::Ptr collection = ci->second;

		for (ConfigCollection::ObjectIterator oi = collection->Objects.begin(); oi != collection->Objects.end(); oi++) {
			JsonRpcMessage::Ptr msg = MakeObjectMessage(oi->second, "config::ObjectCreated", true);
			client->SendMessage(msg);
		}
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectCreatedHandler(ConfigObjectEventArgs::Ptr ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea->Source);
	
	int replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMessage(m_ConfigRpcEndpoint, Endpoint::Ptr(), MakeObjectMessage(object, "config::ObjectCreated", true));
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(ConfigObjectEventArgs::Ptr ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea->Source);
	
	int replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMessage(m_ConfigRpcEndpoint, Endpoint::Ptr(), MakeObjectMessage(object, "config::ObjectRemoved", false));
	}

	return 0;
}

int ConfigRpcComponent::LocalPropertyChangedHandler(ConfigObjectEventArgs::Ptr ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea->Source);
	
	int replicate = 0;
	object->GetPropertyInteger("replicate", &replicate);

	if (replicate) {
		JsonRpcMessage::Ptr msg = MakeObjectMessage(object, "config::PropertyChanged", false);
		cJSON *params = msg->GetParams();

		cJSON *properties = cJSON_CreateObject();
		cJSON_AddItemToObject(params, "properties", properties);

		string value;
		object->GetProperty(ea->Property, &value);

		cJSON_AddStringToObject(properties, ea->Property.c_str(), value.c_str());

		EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
		mgr->SendMessage(m_ConfigRpcEndpoint, Endpoint::Ptr(), msg);
	}

	return 0;
}

int ConfigRpcComponent::RemoteObjectUpdatedHandler(NewMessageEventArgs::Ptr ea)
{
	JsonRpcMessage::Ptr message = ea->Message;
	string name, type, value;
	bool was_null = false;

	if (!message->GetParamString("name", &name))
		return 0;

	if (!message->GetParamString("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object) {
		was_null = true;
		object = make_shared<ConfigObject>(type, name);
	}

	cJSON *properties = message->GetParam("properties");

	if (properties != NULL) {
		for (cJSON *prop = properties->child; prop != NULL; prop = prop->next) {
			if (prop->type != cJSON_String)
				continue;

			object->SetProperty(prop->string, prop->valuestring);
		}
	}

	if (was_null)
		configHive->AddObject(object);

	return 0;
}

int ConfigRpcComponent::RemoteObjectRemovedHandler(NewMessageEventArgs::Ptr ea)
{
	JsonRpcMessage::Ptr message = ea->Message;
	string name, type;
	
	if (!message->GetParamString("name", &name))
		return 0;

	if (!message->GetParamString("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (object.get() == NULL)
		return 0;

	configHive->RemoveObject(object);

	return 0;
}

EXPORT_COMPONENT(ConfigRpcComponent);
