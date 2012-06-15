#include <i2-dyn.h>
//#include <i2-jsonrpc.h>

using std::cout;
using std::endl;

using namespace icinga;

int main(int argc, char **argv)
{
	if (argc < 2) {
		cout << "Syntax: " << argv[0] << " <filename>" << endl;
		return 1;
	}

	for (int i = 0; i < 1; i++) {
		vector<ConfigItem::Ptr> objects = ConfigCompiler::CompileFile(string(argv[1]));

		ConfigVM::ExecuteItems(objects);
	}

/*	ObjectSet<DynamicObject::Ptr>::Iterator it;
	for (it = DynamicObject::GetAllObjects()->Begin(); it != DynamicObject::GetAllObjects()->End(); it++) {
		DynamicObject::Ptr obj = *it;
		cout << "Object, name: " << obj->GetName() << ", type: " << obj->GetType() << endl;

		MessagePart mp(obj->GetConfig());
		cout << mp.ToJsonString() << endl;
	}
*/
	return 0;
}
