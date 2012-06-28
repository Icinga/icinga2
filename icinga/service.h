#ifndef SERVICE_H
#define SERVICE_H

namespace icinga
{

enum ServiceState
{
	StateOK,
	StateWarning,
	StateCritical,
	StateUnknown,
	StateUnreachable,
	StateUncheckable,
};

enum ServiceStateType
{
	StateTypeSoft,
	StateTypeHard
};

struct CheckResult;

class I2_ICINGA_API Service : public ConfigObjectAdapter
{
public:
	Service(const ConfigObject::Ptr& configObject)
		: ConfigObjectAdapter(configObject)
	{ }

	static Service GetByName(string name);

	string GetAlias(void) const;
	Host GetHost(void) const;
	Dictionary::Ptr GetMacros(void) const;
	string GetCheckType(void) const;
	string GetCheckCommand(void) const;
	long GetMaxCheckAttempts(void) const;
	long GetCheckInterval(void) const;
	long GetRetryInterval(void) const;
	Dictionary::Ptr GetDependencies(void) const;

	void SetNextCheck(time_t nextCheck);
	time_t GetNextCheck(void);
	void UpdateNextCheck(void);

	void SetChecker(string checker);
	string GetChecker(void) const;

	bool IsAllowedChecker(string checker) const;

	void SetCurrentCheckAttempt(long attempt);
	long GetCurrentCheckAttempt(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetStateType(ServiceStateType type);
	ServiceStateType GetStateType(void) const;

	void SetLastCheckResult(const Dictionary::Ptr& result);
	Dictionary::Ptr GetLastCheckResult(void) const;

	void SetLastStateChange(time_t ts);
	time_t GetLastStateChange(void) const;

	void SetLastHardStateChange(time_t ts);
	time_t GetLastHardStateChange(void) const;

	void ApplyCheckResult(const CheckResult& cr);

	static ServiceState StateFromString(string state);
	static string StateToString(ServiceState state);

	static ServiceStateType StateTypeFromString(string state);
	static string StateTypeToString(ServiceStateType state);
};

}

#endif /* SERVICE_H */
