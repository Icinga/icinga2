/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef HOST_H
#define HOST_H

#include "icinga/i2-icinga.hpp"
#include "icinga/host.thpp"
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

	std::vector<intrusive_ptr<Service> > GetServices(void) const;
	void AddService(const intrusive_ptr<Service>& service);
	void RemoveService(const intrusive_ptr<Service>& service);

	int GetTotalServices(void) const;

	static HostState CalculateState(ServiceState state);

	virtual HostState GetState(void) const override;
	virtual HostState GetLastState(void) const override;
	virtual HostState GetLastHardState(void) const override;
	virtual int GetSeverity(void) const override;

	virtual bool IsStateOK(ServiceState state) override;
	virtual void SaveLastState(ServiceState state, double timestamp) override;

	static HostState StateFromString(const String& state);
	static String StateToString(HostState state);

	static StateType StateTypeFromString(const String& state);
	static String StateTypeToString(StateType state);

	virtual bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const override;

protected:
	virtual void Stop(bool runtimeRemoved) override;

	virtual void OnAllConfigLoaded(void) override;
	virtual void CreateChildObjects(const Type::Ptr& childType) override;

private:
	mutable boost::mutex m_ServicesMutex;
	std::map<String, intrusive_ptr<Service> > m_Services;

	static void RefreshServicesCache(void);
};

}

#endif /* HOST_H */

#include "icinga/service.hpp"
