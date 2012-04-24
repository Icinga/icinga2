#include "i2-icinga.h"

using namespace icinga;

IcingaApplication::Ptr IcingaComponent::GetIcingaApplication(void) const
{
	return static_pointer_cast<IcingaApplication>(GetApplication());
}

EndpointManager::Ptr IcingaComponent::GetEndpointManager(void) const
{
	IcingaApplication::Ptr app = GetIcingaApplication();

	if (!app)
		return EndpointManager::Ptr();

	return app->GetEndpointManager();
}

ConfigHive::Ptr IcingaComponent::GetConfigHive(void) const
{
	IcingaApplication::Ptr app = GetIcingaApplication();

	if (!app)
		return ConfigHive::Ptr();

	return app->GetConfigHive();
}
