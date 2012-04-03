#ifndef APPLICATION_H
#define APPLICATION_H

namespace icinga {

class Component;

DEFINE_EXCEPTION_CLASS(ComponentLoadException);

class Application : public Object {
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
	void UnloadComponent(const string& name);
	shared_ptr<Component> GetComponent(const string& name);
	void AddComponentSearchDir(const string& componentDirectory);

	const string& GetExeDirectory(void);

	bool IsDebugging(void) const;
	void SigIntHandler(int signum);
};

inline void sigint_handler(int signum)
{
	Application::Instance->SigIntHandler(signum);
}

template<class T>
int application_main(int argc, char **argv)
{
	int result;

	Application::Instance = make_shared<T>();

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);
#endif /* _WIN32 */

	vector<string> args;

	for (int i = 0; i < argc; i++)
		args.push_back(string(argv[i]));

	Application::Instance->SetArguments(args);

	if (Application::Instance->IsDebugging()) {
		result = Application::Instance->Main(args);
	} else {
		try {
			result = Application::Instance->Main(args);
		} catch (const Exception& ex) {
			cout << "---" << endl;

			string klass = typeid(ex).name();

#ifdef HAVE_GCC_ABI_DEMANGLE
			int status;
			char *realname = abi::__cxa_demangle(klass.c_str(), 0, 0, &status);

			if (realname != NULL) {
				klass = string(realname);
				free(realname);
			}
#endif /* HAVE_GCC_ABI_DEMANGLE */

			cout << "Exception: " << klass << endl;
			cout << "Message: " << ex.GetMessage() << endl;

			return EXIT_FAILURE;
		}
	}

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
