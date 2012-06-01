#include <i2-dyn.h>

using namespace icinga;

int main(int argc, char **argv)
{
	ConfigContext ctx;
	ctx.Compile();
	map<pair<string, string>, DConfigObject::Ptr> objects = ctx.GetResult();

	for (auto it = objects.begin(); it != objects.end(); it++) {
		DConfigObject::Ptr obj = it->second;
		cout << "Object, name: " << it->first.second << ", type: " << it->first.first << endl;
		cout << "\t" << obj->GetParents().size() << " parents" << endl;
		cout << "\t" << obj->GetExpressionList()->GetLength() << " top-level exprs" << endl;
	}

	return 0;
}
