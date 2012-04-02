#ifndef CONFIGFILECOMPONENT_H
#define CONFIGFILECOMPONENT_H

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

#endif /* CONFIGFILECOMPONENT_H */
