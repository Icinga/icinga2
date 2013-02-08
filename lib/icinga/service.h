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

#ifndef SERVICE_H
#define SERVICE_H

namespace icinga
{

/**
 * The state of a service.
 *
 * @ingroup icinga
 */
enum ServiceState
{
	StateOK,
	StateWarning,
	StateCritical,
	StateUnknown,
	StateUncheckable,
};

/**
 * The state type of a service.
 *
 * @ingroup icinga
 */
enum ServiceStateType
{
	StateTypeSoft,
	StateTypeHard
};

class CheckResultMessage;

/**
 * An Icinga service.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Service : public DynamicObject
{
public:
	typedef shared_ptr<Service> Ptr;
	typedef weak_ptr<Service> WeakPtr;

	Service(const Dictionary::Ptr& properties);
	~Service(void);

	static bool Exists(const String& name);
	static Service::Ptr GetByName(const String& name);

	static const int DefaultMaxCheckAttempts;
	static const int DefaultCheckInterval;
	static const int CheckIntervalDivisor;

	String GetAlias(void) const;
	Host::Ptr GetHost(void) const;
	Dictionary::Ptr GetMacros(void) const;
	Dictionary::Ptr GetDowntimes(void) const;
	Dictionary::Ptr GetComments(void) const;
	String GetCheckCommand(void) const;
	long GetMaxCheckAttempts(void) const;
	double GetCheckInterval(void) const;
	double GetRetryInterval(void) const;
	Dictionary::Ptr GetHostDependencies(void) const;
	Dictionary::Ptr GetServiceDependencies(void) const;
	Dictionary::Ptr GetGroups(void) const;
	Dictionary::Ptr GetCheckers(void) const;

	set<Host::Ptr> GetParentHosts(void) const;
	set<Service::Ptr> GetParentServices(void) const;

	bool IsReachable(void) const;
	bool IsInDowntime(void) const;

	long GetSchedulingOffset(void);
	void SetSchedulingOffset(long offset);

	void SetFirstCheck(bool first);
	bool GetFirstCheck(void) const;

	void SetNextCheck(double nextCheck);
	double GetNextCheck(void);
	void UpdateNextCheck(void);

	void SetChecker(const String& checker);
	String GetChecker(void) const;

	bool IsAllowedChecker(const String& checker) const;

	void SetCurrentCheckAttempt(long attempt);
	long GetCurrentCheckAttempt(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetStateType(ServiceStateType type);
	ServiceStateType GetStateType(void) const;

	void SetLastCheckResult(const Dictionary::Ptr& result);
	Dictionary::Ptr GetLastCheckResult(void) const;

	void SetLastStateChange(double ts);
	double GetLastStateChange(void) const;

	void SetLastHardStateChange(double ts);
	double GetLastHardStateChange(void) const;

	bool GetEnableActiveChecks(void) const;
	void SetEnableActiveChecks(bool enabled);

	bool GetEnablePassiveChecks(void) const;
	void SetEnablePassiveChecks(bool enabled);

	bool GetForceNextCheck(void) const;
	void SetForceNextCheck(bool forced);

	AcknowledgementType GetAcknowledgement(void);
	void SetAcknowledgement(AcknowledgementType acknowledgement);

	double GetAcknowledgementExpiry(void) const;
	void SetAcknowledgementExpiry(double timestamp);

	void ApplyCheckResult(const Dictionary::Ptr& cr);

	void BeginExecuteCheck(const function<void (void)>& callback);
	void ProcessCheckResult(const Dictionary::Ptr& cr);

	static ServiceState StateFromString(const String& state);
	static String StateToString(ServiceState state);

	static ServiceStateType StateTypeFromString(const String& state);
	static String StateTypeToString(ServiceStateType state);

	static Dictionary::Ptr ResolveDependencies(const Host::Ptr& host, const Dictionary::Ptr& dependencies);

	static boost::signal<void (const Service::Ptr& service, const CheckResultMessage&)> OnCheckResultReceived;
	static boost::signal<void (const Service::Ptr&, const String&)> OnCheckerChanged;
	static boost::signal<void (const Service::Ptr&, const Value&)> OnNextCheckChanged;

protected:
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	void CheckCompletedHandler(const Dictionary::Ptr& scheduleInfo,
	    const ScriptTask::Ptr& task, const function<void (void)>& callback);
};

}

#endif /* SERVICE_H */
