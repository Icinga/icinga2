#include <i2-dyn.h>
#include <i2-jsonrpc.h>

using namespace icinga;

int main(int argc, char **argv)
{
	ConfigContext ctx;
	ctx.Compile();
	set<DConfigObject::Ptr> objects = ctx.GetResult();

	for (auto it = objects.begin(); it != objects.end(); it++) {
		DConfigObject::Ptr obj = *it;
		cout << "Object, name: " << obj->GetName() << ", type: " << obj->GetType() << endl;
		cout << "\t" << obj->GetParents().size() << " parents" << endl;
		cout << "\t" << obj->GetExpressionList()->GetLength() << " top-level exprs" << endl;

		Dictionary::Ptr props = obj->CalculateProperties();
		cout << "\t" << props->GetLength() << " top-level properties" << endl;

		MessagePart mp(props);
		cout << mp.ToJsonString() << endl;
	}

	return 0;
}
