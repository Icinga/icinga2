#include <i2-dyn.h>

using namespace icinga;

bool foogetter(const Object::Ptr& object, string *key)
{
	DynamicObject::Ptr dobj = dynamic_pointer_cast<DynamicObject>(object);
	return dobj->GetConfig()->GetProperty("foo", key);
}

bool foo(const Object::Ptr& object)
{
	DynamicObject::Ptr dobj = dynamic_pointer_cast<DynamicObject>(object);

	string value;
	return dobj->GetConfig()->GetProperty("foo", &value);
}

int main(int argc, char **argv)
{
	for (int i = 0; i < 1000000; i++) {
		DynamicObject::Ptr dobj = make_shared<DynamicObject>();
		dobj->GetConfig()->SetProperty("foo", "bar");
		dobj->Commit();
	}

	ObjectSet::Ptr filtered = make_shared<ObjectSet>(ObjectSet::GetAllObjects(), &foo);
	filtered->Start();

	ObjectMap::Ptr m = make_shared<ObjectMap>(ObjectSet::GetAllObjects(), &foogetter);
	m->Start();

	ObjectMap::Range range = m->GetRange("bar");
	cout << distance(range.first, range.second) << " elements" << endl;

	return 0;
}
