/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HOST_H
#define HOST_H

#include "icinga/i2-icinga.hpp"
#include "icinga/host-ti.hpp"
#include "icinga/macroresolver.hpp"
#include "icinga/checkresult.hpp"

namespace icinga
{

class Service;

/**
 * An Icinga host.
 *
 * @ingroup icinga
 */
class Host final : public ObjectImpl<Host>, public MacroResolver
{
public:
	DECLARE_OBJECT(Host);
	DECLARE_OBJECTNAME(Host);

	intrusive_ptr<Service> GetServiceByShortName(const Value& name);

	std::vector<intrusive_ptr<Service> > GetServices() const;
	void AddService(const intrusive_ptr<Service>& service);
	void RemoveService(const intrusive_ptr<Service>& service);

	int GetTotalServices() const;

	static HostState CalculateState(ServiceState state);

	HostState GetState() const override;
	HostState GetLastState() const override;
	HostState GetLastHardState() const override;
	int GetSeverity() const override;

	bool IsStateOK(ServiceState state) const override;
	void SaveLastState(ServiceState state, double timestamp) override;

	static HostState StateFromString(const String& state);
	static String StateToString(HostState state);

	static StateType StateTypeFromString(const String& state);
	static String StateTypeToString(StateType state);

	bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const override;

protected:
	void Stop(bool runtimeRemoved) override;

	void OnAllConfigLoaded() override;
	void CreateChildObjects(const Type::Ptr& childType) override;

private:
	mutable std::mutex m_ServicesMutex;
	std::map<String, intrusive_ptr<Service> > m_Services;

	static void RefreshServicesCache();
};

}

#endif /* HOST_H */

#include "icinga/service.hpp"
