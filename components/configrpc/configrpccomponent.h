#ifndef CONFIGRPCCOMPONENT_H
#define CONFIGRPCCOMPONENT_H

namespace icinga
{

class ConfigRpcComponent : public Component
{
private:
	VirtualEndpoint::Ptr m_ConfigRpcEndpoint;

	IcingaApplication::Ptr GetIcingaApplication(void);

	int LocalObjectCreatedHandler(const ConfigObjectEventArgs& ea);
	int LocalObjectRemovedHandler(const ConfigObjectEventArgs& ea);
	int LocalPropertyChangedHandler(const ConfigObjectEventArgs& ea);

	int FetchObjectsHandler(const NewRequestEventArgs& ea);
	int RemoteObjectUpdatedHandler(const NewRequestEventArgs& ea);
	int RemoteObjectRemovedHandler(const NewRequestEventArgs& ea);

	JsonRpcRequest MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* CONFIGRPCCOMPONENT_H */
