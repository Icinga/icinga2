/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "icinga/i2-icinga.h"
#include "icinga/macroresolver.h"
//#include "base/i2-base.h"
#include "base/array.h"
#include "base/dynamicobject.h"
#include "base/dictionary.h"

namespace icinga
{

class Service;

/**
 * The state of a host.
 *
 * @ingroup icinga
 */
enum HostState
{
	HostUp = 0,
	HostDown = 1,
	HostUnreachable = 2
};

/**
 * The state of a service.
 *
 * @ingroup icinga
 */
enum ServiceState
{
	StateOK = 0,
	StateWarning = 1,
	StateCritical = 2,
	StateUnknown = 3,
	StateUncheckable = 4,
};

/**
 * The state type of a host or service.
 *
 * @ingroup icinga
 */
enum StateType
{
	StateTypeSoft = 0,
	StateTypeHard = 1
};

/**
 * An Icinga host.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Host : public DynamicObject, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(Host);

	explicit Host(const Dictionary::Ptr& serializedUpdate);
	~Host(void);

	static Host::Ptr GetByName(const String& name);

	String GetDisplayName(void) const;
	Array::Ptr GetGroups(void) const;

	Dictionary::Ptr GetMacros(void) const;
	Array::Ptr GetHostDependencies(void) const;
	Array::Ptr GetServiceDependencies(void) const;
	String GetHostCheck(void) const;

	shared_ptr<Service> GetHostCheckService(void) const;
	std::set<Host::Ptr> GetParentHosts(void) const;
	std::set<shared_ptr<Service> > GetParentServices(void) const;

	bool IsReachable() const;

	shared_ptr<Service> GetServiceByShortName(const Value& name) const;

	std::set<shared_ptr<Service> > GetServices(void) const;
	int GetTotalServices(void) const;
	static void InvalidateServicesCache(void);

	static Value ValidateServiceDictionary(const String& location, const Dictionary::Ptr& attrs);

	static HostState CalculateState(ServiceState state, bool reachable);

	HostState GetState(void) const;
	StateType GetStateType(void) const;
	HostState GetLastState(void) const;
	HostState GetLastHardState(void) const;
	StateType GetLastStateType(void) const;
	double GetLastStateChange(void) const;
	double GetLastHardStateChange(void) const;
	double GetLastStateUp(void) const;
	double GetLastStateDown(void) const;
	double GetLastStateUnreachable(void) const;

	static String StateToString(HostState state);

	virtual bool ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const;
protected:
	virtual void OnRegistrationCompleted(void);
	virtual void OnAttributeChanged(const String& name);

private:
	Attribute<String> m_DisplayName;
	Attribute<Array::Ptr> m_HostGroups;
	Attribute<Dictionary::Ptr> m_Macros;
	Attribute<Array::Ptr> m_HostDependencies;
	Attribute<Array::Ptr> m_ServiceDependencies;
	Attribute<String> m_HostCheck;
	Dictionary::Ptr m_SlaveServices;

	void UpdateSlaveServices(void);

	static void RefreshServicesCache(void);
};

}

#endif /* HOST_H */
