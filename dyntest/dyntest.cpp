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
	int num = atoi(argv[1]);
	int num_obs = atoi(argv[2]);

	time_t st, et;

	time(&st);
	vector<DynamicObject::Ptr> objects;
	for (int i = 0; i < num; i++) {
		DynamicObject::Ptr dobj = make_shared<DynamicObject>();
		dobj->GetConfig()->SetProperty("foo", "bar");
		objects.push_back(dobj);
	}
	time(&et);
	cout << "Creating objects: " << et - st << " seconds" << endl;

	time(&st);
	for (vector<DynamicObject::Ptr>::iterator it = objects.begin(); it != objects.end(); it++) {
		(*it)->Commit();
	}
	time(&et);
	cout << "Committing objects: " << et - st << " seconds" << endl;

	time(&st);
	Dictionary::Ptr obs = make_shared<Dictionary>();
	for (int a = 0; a < num_obs; a++) {
		ObjectSet::Ptr os = make_shared<ObjectSet>(ObjectSet::GetAllObjects(), &foo);
		os->Start();
		obs->AddUnnamedProperty(os);
	}
	time(&et);
	cout << "Creating objectsets: " << et - st << " seconds" << endl;

	time(&st);
	ObjectMap::Ptr m = make_shared<ObjectMap>(ObjectSet::GetAllObjects(), &foogetter);
	m->Start();
	time(&et);
	cout << "Creating objectmap: " << et - st << " seconds" << endl;

	time(&st);
	ObjectMap::Range range = m->GetRange("bar");
	time(&et);
	cout << "Retrieving objects from map: " << et - st << " seconds" << endl;
	cout << distance(range.first, range.second) << " elements" << endl;

	return 0;
}
