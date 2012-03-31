#ifndef I2_CONFIGFILECOMPONENT_H
#define I2_CONFIGFILECOMPONENT_H

namespace icinga
{

class ConfigFileComponent : public Component
{
public:
	typedef shared_ptr<ConfigFileComponent> RefType;
	typedef weak_ptr<ConfigFileComponent> WeakRefType;

	virtual string GetName(void);
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* I2_CONFIGFILECOMPONENT_H */