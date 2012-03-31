#ifndef I2_COMPONENT_H
#define I2_COMPONENT_H

namespace icinga
{

class Component : public Object
{
private:
	Application::WeakRefType m_Application;

public:
	typedef shared_ptr<Component> RefType;
	typedef weak_ptr<Component> WeakRefType;

	void SetApplication(Application::WeakRefType application);
	Application::RefType GetApplication(void);

	virtual string GetName(void) = 0;
	virtual void Start(ConfigObject::RefType componentConfig) = 0;
	virtual void Stop(void) = 0;
};

#define EXPORT_COMPONENT(klass) \
	extern "C" I2_EXPORT icinga::Component *CreateComponent(void)	\
	{														\
		return new klass();									\
	}

}

#endif /* I2_COMPONENT_H */
