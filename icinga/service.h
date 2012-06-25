#ifndef SERVICE_H
#define SERVICE_H

namespace icinga
{

enum ServiceState
{
	StateOK,
	StateWarning,
	StateCritical,
	StateUnreachable,
	StateUncheckable,
	StateUnknown
};

enum ServiceStateType
{
	StateTypeHard,
	StateTypeSoft
};

struct CheckResult;

class I2_ICINGA_API Service : public ConfigObjectAdapter
{
public:
	Service(const ConfigObject::Ptr& configObject)
		: ConfigObjectAdapter(configObject)
	{ }

	string GetDisplayName(void) const;
	Host GetHost(void) const;
	Dictionary::Ptr GetMacros(void) const;
	string GetCheckType(void) const;
	string GetCheckCommand(void) const;
	long GetMaxCheckAttempts(void) const;
	long GetCheckInterval(void) const;
	long GetRetryInterval(void) const;

	void SetNextCheck(time_t nextCheck);
	time_t GetNextCheck(void);

	void SetChecker(string checker);
	string GetChecker(void) const;

	void SetCurrentCheckAttempt(long attempt);
	long GetCurrentCheckAttempt(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetStateType(ServiceStateType type);
	ServiceStateType GetStateType(void) const;

	void ApplyCheckResult(const CheckResult& cr);
};

}

#endif /* SERVICE_H */
