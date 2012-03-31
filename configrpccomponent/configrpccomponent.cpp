#include "i2-configrpccomponent.h"

using namespace icinga;
using std::dynamic_pointer_cast;

IcingaApplication::RefType ConfigRpcComponent::GetIcingaApplication(void)
{
	return dynamic_pointer_cast<IcingaApplication>(GetApplication());
}

string ConfigRpcComponent::GetName(void)
{
	return "configcomponent";
}

void ConfigRpcComponent::Start(void)
{
	IcingaApplication::RefType icingaApp = GetIcingaApplication();

	if (icingaApp.get() == NULL)
		throw exception(/*"Component loaded by incompatible application."*/);

	ConnectionManager::RefType connectionManager = icingaApp->GetConnectionManager();
	ConfigHive::RefType configHive = icingaApp->GetConfigHive();

	if (GetConfig()->GetPropertyInteger("configSource") != 0) {
		connectionManager->RegisterMethod("config::FetchObjects", bind_weak(&ConfigRpcComponent::FetchObjectsHandler, shared_from_this()));

		configHive->OnObjectCreated.bind(bind_weak(&ConfigRpcComponent::LocalObjectCreatedHandler, shared_from_this()));
		configHive->OnObjectRemoved.bind(bind_weak(&ConfigRpcComponent::LocalObjectRemovedHandler, shared_from_this()));
		configHive->OnPropertyChanged.bind(bind_weak(&ConfigRpcComponent::LocalPropertyChangedHandler, shared_from_this()));
	}

	connectionManager->RegisterMethod("config::ObjectCreated", bind_weak(&ConfigRpcComponent::RemoteObjectCreatedHandler, shared_from_this()));
	connectionManager->RegisterMethod("config::ObjectRemoved", bind_weak(&ConfigRpcComponent::RemoteObjectRemovedHandler, shared_from_this()));
	connectionManager->RegisterMethod("config::PropertyChanged", bind_weak(&ConfigRpcComponent::RemotePropertyChangedHandler, shared_from_this()));
}

void ConfigRpcComponent::Stop(void)
{
	// TODO: implement
}

JsonRpcMessage::RefType ConfigRpcComponent::MakeObjectMessage(const ConfigObject::RefType& object, string method, bool includeProperties)
{
	JsonRpcMessage::RefType msg = new_object<JsonRpcMessage>();
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

int ConfigRpcComponent::FetchObjectsHandler(NewMessageEventArgs::RefType ea)
{
	JsonRpcClient::RefType client = static_pointer_cast<JsonRpcClient>(ea->Source);
	ConfigHive::RefType configHive = GetIcingaApplication()->GetConfigHive();

	for (ConfigHive::TypeIterator ti = configHive->Objects.begin(); ti != configHive->Objects.end(); ti++) {
		for (ConfigHive::ObjectIterator oi = ti->second.begin(); oi != ti->second.end(); oi++) {
			JsonRpcMessage::RefType msg = MakeObjectMessage(oi->second, "config::ObjectCreated", true);
			client->SendMessage(msg);
		}
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectCreatedHandler(ConfigHiveEventArgs::RefType ea)
{
	ConnectionManager::RefType connectionManager = GetIcingaApplication()->GetConnectionManager();
	connectionManager->SendMessage(MakeObjectMessage(ea->ConfigObject, "config::ObjectCreated", true));

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(ConfigHiveEventArgs::RefType ea)
{
	ConnectionManager::RefType connectionManager = GetIcingaApplication()->GetConnectionManager();
	connectionManager->SendMessage(MakeObjectMessage(ea->ConfigObject, "config::ObjectRemoved", false));

	return 0;
}

int ConfigRpcComponent::LocalPropertyChangedHandler(ConfigHiveEventArgs::RefType ea)
{
	JsonRpcMessage::RefType msg = MakeObjectMessage(ea->ConfigObject, "config::ObjectRemoved", false);
	cJSON *params = msg->GetParams();
	cJSON_AddStringToObject(params, "property", ea->Property.c_str());
	string value = ea->ConfigObject->GetProperty(ea->Property);
	cJSON_AddStringToObject(params, "value", value.c_str());

	ConnectionManager::RefType connectionManager = GetIcingaApplication()->GetConnectionManager();
	connectionManager->SendMessage(msg);

	return 0;
}

int ConfigRpcComponent::RemoteObjectCreatedHandler(NewMessageEventArgs::RefType ea)
{
	JsonRpcMessage::RefType message = ea->Message;

	// TODO: update hive
	return 0;
}

int ConfigRpcComponent::RemoteObjectRemovedHandler(NewMessageEventArgs::RefType ea)
{
	JsonRpcMessage::RefType message = ea->Message;
	string name, type;
	
	if (!message->GetParamString("name", &name))
		return 0;

	if (!message->GetParamString("type", &type))
		return 0;

	ConfigHive::RefType configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::RefType object = configHive->GetObject(type, name);

	if (object.get() == NULL)
		return 0;

	configHive->RemoveObject(object);

	return 0;
}

int ConfigRpcComponent::RemotePropertyChangedHandler(NewMessageEventArgs::RefType ea)
{
	JsonRpcMessage::RefType message = ea->Message;
	string name, type, property, value;

	if (!message->GetParamString("name", &name))
		return 0;

	if (!message->GetParamString("type", &type))
		return 0;

	if (!message->GetParamString("property", &property))
		return 0;

	if (!message->GetParamString("value", &value))
		return 0;

	ConfigHive::RefType configHive = GetIcingaApplication()->GetConfigHive();
	ConfigObject::RefType object = configHive->GetObject(type, name);

	if (object.get() == NULL)
		return 0;

	object->SetProperty(property, value);

	return 0;
}


EXPORT_COMPONENT(ConfigRpcComponent);
