#ifndef I2_APPLICATION_H
#define I2_APPLICATION_H

namespace icinga {

using std::vector;
using std::string;

class Application : public Object {
private:
	bool m_ShuttingDown;

public:
	typedef shared_ptr<Application> RefType;
	typedef weak_ptr<Application> WeakRefType;

	static Application::RefType Instance;

	Application(void);
	~Application(void);

	virtual int Main(const vector<string>& args) = 0;

	void RunEventLoop(void);
	void Shutdown(void);
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
