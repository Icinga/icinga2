#ifndef COMPONENT_H
#define COMPONENT_H

namespace icinga
{

class Component : public Object
{
private:
	Application::WeakRefType m_Application;
	ConfigObject::RefType m_Config;

public:
	typedef shared_ptr<Component> RefType;
	typedef weak_ptr<Component> WeakRefType;

	void SetApplication(const Application::WeakRefType& application);
	Application::RefType GetApplication(void);

	void SetConfig(ConfigObject::RefType componentConfig);
	ConfigObject::RefType GetConfig(void);

	virtual string GetName(void) = 0;
	virtual void Start(void) = 0;
	virtual void Stop(void) = 0;
};

#define EXPORT_COMPONENT(klass) \
	extern "C" I2_EXPORT icinga::Component *CreateComponent(void)	\
	{														\
		return new klass();									\
	}

}

#endif /* COMPONENT_H */
