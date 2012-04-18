#ifndef APPLICATION_H
#define APPLICATION_H

namespace icinga {

class Component;

DEFINE_EXCEPTION_CLASS(ComponentLoadException);

class I2_BASE_API Application : public Object {
private:
	bool m_ShuttingDown;
	ConfigHive::Ptr m_ConfigHive;
	map< string, shared_ptr<Component> > m_Components;
	vector<string> m_Arguments;
	bool m_Debugging;

public:
	typedef shared_ptr<Application> Ptr;
	typedef weak_ptr<Application> WeakPtr;

	static Application::Ptr Instance;

	Application(void);
	~Application(void);

	virtual int Main(const vector<string>& args) = 0;

	void SetArguments(const vector<string>& arguments);
	const vector<string>& GetArguments(void);

	void RunEventLoop(void);
	bool Daemonize(void);
	void Shutdown(void);

	void Log(const char *format, ...);

	ConfigHive::Ptr GetConfigHive(void);

	shared_ptr<Component> LoadComponent(const string& path, const ConfigObject::Ptr& componentConfig);
	void RegisterComponent(shared_ptr<Component> component);
	void UnregisterComponent(shared_ptr<Component> component);
	shared_ptr<Component> GetComponent(const string& name);
	void AddComponentSearchDir(const string& componentDirectory);

	const string& GetExeDirectory(void);

	bool IsDebugging(void) const;
	void SigIntHandler(int signum);
};

}

int I2_EXPORT application_main(int argc, char **argv, icinga::Application *instance);

#define IMPLEMENT_ENTRY_POINT(klass)					\
	int main(int argc, char **argv) {					\
		klass *instance = new klass();					\
		return application_main(argc, argv, instance);	\
	}

#endif /* APPLICATION_H */
