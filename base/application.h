#ifndef I2_APPLICATION_H
#define I2_APPLICATION_H

#include <map>

namespace icinga {

using std::vector;
using std::map;
using std::string;

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

	shared_ptr<Component> LoadComponent(string name);
	void UnloadComponent(string name);
	shared_ptr<Component> GetComponent(string name);
};

template<class T>
int i2_main(int argc, char **argv)
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
		return i2_main<klass>(argc, argv);	\
	}

}

#endif /* I2_APPLICATION_H */
