#ifndef APPLICATION_H
#define APPLICATION_H

namespace icinga {

class Component;

class Application : public Object {
private:
	bool m_ShuttingDown;
	ConfigHive::RefType m_ConfigHive;
	map< string, shared_ptr<Component> > m_Components;

public:
	typedef shared_ptr<Application> RefType;
	typedef weak_ptr<Application> WeakRefType;

	static Application::RefType Instance;

	Application(void);
	~Application(void);

	virtual int Main(const vector<string>& args) = 0;

	void RunEventLoop(void);
	bool Daemonize(void);
	void Shutdown(void);

	void Log(const char *format, ...);

	ConfigHive::RefType GetConfigHive(void);

	shared_ptr<Component> LoadComponent(string path, ConfigObject::RefType componentConfig);
	void UnloadComponent(string name);
	shared_ptr<Component> GetComponent(string name);
};

template<class T>
int application_main(int argc, char **argv)
{
	int result;

	Application::Instance = new_object<T>();

	vector<string> args;

	for (int i = 0; i < argc; i++)
		args.push_back(string(argv[i]));

	result = Application::Instance->Main(args);

	Application::Instance.reset();

	assert(Object::ActiveObjects == 0);

	return result;
}

#define SET_START_CLASS(klass)				\
	int main(int argc, char **argv) {		\
		return application_main<klass>(argc, argv);	\
	}

}

#endif /* APPLICATION_H */
