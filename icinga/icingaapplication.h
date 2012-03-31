#ifndef I2_ICINGAAPPLICATION_H
#define I2_ICINGAAPPLICATION_H

namespace icinga
{

class IcingaApplication : public Application
{
private:
	ConnectionManager::RefType m_ConnectionManager;

public:
	typedef shared_ptr<IcingaApplication> RefType;
	typedef weak_ptr<IcingaApplication> WeakRefType;

	IcingaApplication(void);

	virtual int Main(const vector<string>& args);

	virtual ConnectionManager::RefType GetConnectionManager(void);
};

}

#endif /* I2_ICINGAAPPLICATION_H */
