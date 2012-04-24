#ifndef ICINGACOMPONENT_H
#define ICINGACOMPONENT_H

namespace icinga
{

class I2_ICINGA_API IcingaComponent : public Component
{
protected:
	IcingaApplication::Ptr GetIcingaApplication(void) const;
	EndpointManager::Ptr GetEndpointManager(void) const;
	ConfigHive::Ptr GetConfigHive(void) const;
};

}

#endif /* ICINGACOMPONENT_H */
