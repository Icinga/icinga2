#ifndef CONFIGRPCCOMPONENT_H
#define CONFIGRPCCOMPONENT_H

namespace icinga
{

class ConfigRpcComponent : public Component
{
private:
	IcingaApplication::Ptr GetIcingaApplication(void);

	int FetchObjectsHandler(NewMessageEventArgs::Ptr ea);

	int LocalObjectCreatedHandler(ConfigObjectEventArgs::Ptr ea);
	int LocalObjectRemovedHandler(ConfigObjectEventArgs::Ptr ea);
	int LocalPropertyChangedHandler(ConfigObjectEventArgs::Ptr ea);

	int RemoteObjectCreatedHandler(NewMessageEventArgs::Ptr ea);
	int RemoteObjectRemovedHandler(NewMessageEventArgs::Ptr ea);
	int RemotePropertyChangedHandler(NewMessageEventArgs::Ptr ea);

	JsonRpcMessage::Ptr MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties);

public:
	virtual string GetName(void);
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* CONFIGRPCCOMPONENT_H */
