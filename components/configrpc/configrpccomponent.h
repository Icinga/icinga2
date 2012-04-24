#ifndef CONFIGRPCCOMPONENT_H
#define CONFIGRPCCOMPONENT_H

namespace icinga
{

class ConfigRpcComponent : public IcingaComponent
{
private:
	VirtualEndpoint::Ptr m_ConfigRpcEndpoint;

	int NewEndpointHandler(const NewEndpointEventArgs& ea);
	int SessionEstablishedHandler(const EventArgs& ea);

	int LocalObjectCreatedHandler(const EventArgs& ea);
	int LocalObjectRemovedHandler(const EventArgs& ea);
	int LocalPropertyChangedHandler(const PropertyChangedEventArgs& ea);

	int FetchObjectsHandler(const NewRequestEventArgs& ea);
	int RemoteObjectUpdatedHandler(const NewRequestEventArgs& ea);
	int RemoteObjectRemovedHandler(const NewRequestEventArgs& ea);

	static JsonRpcRequest MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties);

	static bool ShouldReplicateObject(const ConfigObject::Ptr& object);
public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* CONFIGRPCCOMPONENT_H */
