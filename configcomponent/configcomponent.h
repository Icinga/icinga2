#ifndef I2_CONFIGCOMPONENT_H
#define I2_CONFIGCOMPONENT_H

namespace icinga
{

class ConfigComponent : public Component
{
private:
	IcingaApplication::RefType GetIcingaApplication(void);

	int FetchObjectsHandler(NewMessageEventArgs::RefType ea);

	int LocalObjectCreatedHandler(ConfigHiveEventArgs::RefType ea);
	int LocalObjectRemovedHandler(ConfigHiveEventArgs::RefType ea);
	int LocalPropertyChangedHandler(ConfigHiveEventArgs::RefType ea);

	int RemoteObjectCreatedHandler(NewMessageEventArgs::RefType ea);
	int RemoteObjectRemovedHandler(NewMessageEventArgs::RefType ea);
	int RemotePropertyChangedHandler(NewMessageEventArgs::RefType ea);

	JsonRpcMessage::RefType MakeObjectMessage(const ConfigObject::RefType& object, string method, bool includeProperties);

public:
	virtual string GetName(void);
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* I2_CONFIGCOMPONENT_H */
