#include "i2-configrpccomponent.h"

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

	ConnectionManager::Ptr connectionManager = icingaApp->GetConnectionManager();
	ConfigHive::Ptr configHive = icingaApp->GetConfigHive();

	int configSource;
	if (GetConfig()->GetPropertyInteger("configSource", &configSource) && configSource != 0) {
		connectionManager->RegisterMethod("config::FetchObjects", bind_weak(&ConfigRpcComponent::FetchObjectsHandler, shared_from_this()));

		configHive->OnObjectCreated += bind_weak(&ConfigRpcComponent::LocalObjectCreatedHandler, shared_from_this());
		configHive->OnObjectRemoved += bind_weak(&ConfigRpcComponent::LocalObjectRemovedHandler, shared_from_this());
		configHive->OnPropertyChanged += bind_weak(&ConfigRpcComponent::LocalPropertyChangedHandler, shared_from_this());
	}

	connectionManager->RegisterMethod("config::ObjectCreated", bind_weak(&ConfigRpcComponent::RemoteObjectCreatedHandler, shared_from_this()));
	connectionManager->RegisterMethod("config::ObjectRemoved", bind_weak(&ConfigRpcComponent::RemoteObjectRemovedHandler, shared_from_this()));
	connectionManager->RegisterMethod("config::PropertyChanged", bind_weak(&ConfigRpcComponent::RemotePropertyChangedHandler, shared_from_this()));
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
		for (ConfigObject::ParameterIterator pi = object->Properties.begin(); pi != object->Properties.end(); pi++) {
			cJSON_AddStringToObject(params, pi->first.c_str(), pi->second.c_str());
		}
	}

	return msg;
}

int ConfigRpcComponent::FetchObjectsHandler(NewMessageEventArgs::Ptr ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea->Source);
	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();

	for (ConfigHive::TypeIterator ti = configHive->Objects.begin(); ti != configHive->Objects.end(); ti++) {
		for (ConfigHive::ObjectIterator oi = ti->second.begin(); oi != ti->second.end(); oi++) {
			JsonRpcMessage::Ptr msg = MakeObjectMessage(oi->second, "config::ObjectCreated", true);
			client->SendMessage(msg);
		}
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectCreatedHandler(ConfigHiveEventArgs::Ptr ea)
{
	ConnectionManager::Ptr connectionManager = GetIcingaApplication()->GetConnectionManager();
	connectionManager->SendMessage(MakeObjectMessage(ea->Object, "config::ObjectCreated", true));

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(ConfigHiveEventArgs::Ptr ea)
{
	ConnectionManager::Ptr connectionManager = GetIcingaApplication()->GetConnectionManager();
	connectionManager->SendMessage(MakeObjectMessage(ea->Object, "config::ObjectRemoved", false));

	return 0;
}

int ConfigRpcComponent::LocalPropertyChangedHandler(ConfigHiveEventArgs::Ptr ea)
{
	JsonRpcMessage::Ptr msg = MakeObjectMessage(ea->Object, "config::ObjectRemoved", false);
	cJSON *params = msg->GetParams();
	cJSON_AddStringToObject(params, "property", ea->Property.c_str());
	string value;
	ea->Object->GetProperty(ea->Property, &value);
	cJSON_AddStringToObject(params, "value", value.c_str());

	ConnectionManager::Ptr connectionManager = GetIcingaApplication()->GetConnectionManager();
	connectionManager->SendMessage(msg);

	return 0;
}

int ConfigRpcComponent::RemoteObjectCreatedHandler(NewMessageEventArgs::Ptr ea)
{
	JsonRpcMessage::Ptr message = ea->Message;

	// TODO: update hive
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

int ConfigRpcComponent::RemotePropertyChangedHandler(NewMessageEventArgs::Ptr ea)
{
	JsonRpcMessage::Ptr message = ea->Message;
	string name, type, property, value;

	if (!message->GetParamString("name", &name))
		return 0;

	if (!message->GetParamString("type", &type))
		return 0;

	if (!message->GetParamString("property", &property))
		return 0;

	if (!message->GetParamString("value", &value))
		return 0;

	ConfigHive::Ptr configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (object.get() == NULL)
		return 0;

	object->SetProperty(property, value);

	return 0;
}


EXPORT_COMPONENT(ConfigRpcComponent);
