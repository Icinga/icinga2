#ifndef APPLICATION_H
#define APPLICATION_H

namespace icinga {

class Component;

class Application : public Object {
private:
	bool m_ShuttingDown;
	ConfigHive::Ptr m_ConfigHive;
	map< string, shared_ptr<Component> > m_Components;
	vector<string> m_Arguments;

public:
	typedef shared_ptr<Application> Ptr;
	typedef weak_ptr<Application> WeakPtr;

	static Application::Ptr Instance;

	Application(void);
	~Application(void);

	virtual int Main(const vector<string>& args) = 0;

	void SetArguments(const vector<string>& arguments);
	vector<string>& GetArguments(void);

	void RunEventLoop(void);
	bool Daemonize(void);
	void Shutdown(void);

	void Log(const char *format, ...);

	ConfigHive::Ptr GetConfigHive(void);

	shared_ptr<Component> LoadComponent(string path, ConfigObject::Ptr componentConfig);
	void UnloadComponent(string name);
	shared_ptr<Component> GetComponent(string name);
	void AddComponentSearchDir(string componentDirectory);

	string GetExeDirectory(void);
};

template<class T>
int application_main(int argc, char **argv)
{
	int result;

	Application::Instance = new_object<T>();

	vector<string> args;

	for (int i = 0; i < argc; i++)
		args.push_back(string(argv[i]));

	Application::Instance->SetArguments(args);

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
