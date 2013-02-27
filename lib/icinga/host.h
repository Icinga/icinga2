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

namespace icinga
{

class Service;

/**
 * An Icinga host.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Host : public DynamicObject
{
public:
	typedef shared_ptr<Host> Ptr;
	typedef weak_ptr<Host> WeakPtr;

	Host(const Dictionary::Ptr& properties);
	~Host(void);

	static bool Exists(const String& name);
	static Host::Ptr GetByName(const String& name);

	String GetDisplayName(void) const;
	Dictionary::Ptr GetGroups(void) const;

	Dictionary::Ptr GetMacros(void) const;
	Dictionary::Ptr GetHostDependencies(void) const;
	Dictionary::Ptr GetServiceDependencies(void) const;
	String GetHostCheck(void) const;

	static Dictionary::Ptr CalculateDynamicMacros(const Host::Ptr& self);

	static shared_ptr<Service> GetHostCheckService(const Host::Ptr& self);
	static set<Host::Ptr> GetParentHosts(const Host::Ptr& self);
	static set<shared_ptr<Service> > GetParentServices(const Host::Ptr& self);

	static bool IsReachable(const Host::Ptr& self);

	static shared_ptr<Service> GetServiceByShortName(const Host::Ptr& self, const Value& name);

	set<shared_ptr<Service> > GetServices(void) const;
	static void RefreshServicesCache(void);

	static void ValidateServiceDictionary(const ScriptTask::Ptr& task,
	    const std::vector<icinga::Value>& arguments);

protected:
	virtual void OnRegistrationCompleted(void);
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	Attribute<String> m_DisplayName;
	Attribute<Dictionary::Ptr> m_HostGroups;
	Attribute<Dictionary::Ptr> m_Macros;
	Attribute<Dictionary::Ptr> m_HostDependencies;
	Attribute<Dictionary::Ptr> m_ServiceDependencies;
	Attribute<String> m_HostCheck;
	Dictionary::Ptr m_SlaveServices;

	static boost::mutex m_ServiceMutex;
	static map<String, map<String, weak_ptr<Service> > > m_ServicesCache;

	static void UpdateSlaveServices(const Host::Ptr& self);

	static void ValidateServicesCache(void);
};

}

#endif /* HOST_H */
