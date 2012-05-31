#include <i2-dyn.h>

using namespace icinga;

bool propgetter(string prop, const Object::Ptr& object, string *key)
{
	DynamicObject::Ptr dobj = dynamic_pointer_cast<DynamicObject>(object);
	return dobj->GetConfig()->GetProperty(prop, key);
}

int main(int argc, char **argv)
{
	for (int i = 0; i < 10000; i++) {
		stringstream sname;
		sname << "foo" << i;

		DynamicObject::Ptr dobj = make_shared<DynamicObject>();
		dobj->GetConfig()->SetProperty("type", "process");
		dobj->GetConfig()->SetProperty("name", sname.str());
		dobj->Commit();
	}

 	ObjectMap::Ptr byType = make_shared<ObjectMap>(ObjectSet::GetAllObjects(),
	    bind(&propgetter, "type", _1, _2));
	byType->Start();

 	ObjectMap::Ptr byName = make_shared<ObjectMap>(ObjectSet::GetAllObjects(),
	    bind(&propgetter, "name", _1, _2));
	byName->Start();

	ObjectMap::Range processes = byType->GetRange("process");
	cout << distance(processes.first, processes.second) << " processes" << endl;

	ObjectMap::Range foo55 = byName->GetRange("foo55");
	cout << distance(foo55.first, foo55.second) << " foo55s" << endl;

	return 0;
}
