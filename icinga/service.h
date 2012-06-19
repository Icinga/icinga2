#ifndef SERVICE_H
#define SERVICE_H

namespace icinga
{

class I2_ICINGA_API Service : public ConfigObjectAdapter
{
public:
	Service(const ConfigObject::Ptr& configObject)
		: ConfigObjectAdapter(configObject), m_NextCheck(-1)
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

private:
	time_t m_NextCheck;
};

}

#endif /* SERVICE_H */
