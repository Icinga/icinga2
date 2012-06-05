#include "i2-dyn.h"

using namespace icinga;

DConfigObject::DConfigObject(string type, string name, long debuginfo)
	: m_Type(type), m_Name(name), m_DebugInfo(debuginfo)
{
}

string DConfigObject::GetType(void) const
{
	return m_Type;
}

string DConfigObject::GetName(void) const
{
	return m_Name;
}

ExpressionList::Ptr DConfigObject::GetExpressionList(void) const
{
	return m_ExpressionList;
}

void DConfigObject::SetExpressionList(const ExpressionList::Ptr& exprl)
{
	m_ExpressionList = exprl;
}

vector<string> DConfigObject::GetParents(void) const
{
	return m_Parents;
}

void DConfigObject::AddParent(string parent)
{
	m_Parents.push_back(parent);
}

Dictionary::Ptr DConfigObject::CalculateProperties(void) const
{
	Dictionary::Ptr result = make_shared<Dictionary>();

	vector<string>::const_iterator it;
	for (it = m_Parents.begin(); it != m_Parents.end(); it++) {
		DConfigObject::Ptr parent = DConfigObject::GetObject(GetType(), *it);

		if (!parent) {
			stringstream message;
			message << "Parent object '" << *it << "' does not exist (in line " << m_DebugInfo << ")";
			throw domain_error(message.str());
		}

		parent->GetExpressionList()->Execute(result);
	}

	m_ExpressionList->Execute(result);

	return result;
}

ObjectSet<DConfigObject::Ptr>::Ptr DConfigObject::GetAllObjects(void)
{
	static ObjectSet<DConfigObject::Ptr>::Ptr allObjects;

        if (!allObjects) {
                allObjects = make_shared<ObjectSet<DConfigObject::Ptr> >();
                allObjects->Start();
        }

        return allObjects;
}

bool DConfigObject::GetTypeAndName(const DConfigObject::Ptr& object, pair<string, string> *key)
{
	*key = make_pair(object->GetType(), object->GetName());

	return true;
}

ObjectMap<pair<string, string>, DConfigObject::Ptr>::Ptr DConfigObject::GetObjectsByTypeAndName(void)
{
	static ObjectMap<pair<string, string>, DConfigObject::Ptr>::Ptr tnmap;

	if (!tnmap) {
		tnmap = make_shared<ObjectMap<pair<string, string>, DConfigObject::Ptr> >(GetAllObjects(), &DConfigObject::GetTypeAndName);
		tnmap->Start();
	}

	return tnmap;
}

void DConfigObject::AddObject(DConfigObject::Ptr object)
{
	GetAllObjects()->AddObject(object);
}

void DConfigObject::RemoveObject(DConfigObject::Ptr object)
{
	throw logic_error("not implemented.");
}

DConfigObject::Ptr DConfigObject::GetObject(string type, string name)
{
	ObjectMap<pair<string, string>, DConfigObject::Ptr>::Range range;
	range = GetObjectsByTypeAndName()->GetRange(make_pair(type, name));

	assert(distance(range.first, range.second) <= 1);

	if (range.first == range.second)
		return DConfigObject::Ptr();
	else
		return range.first->second;
}
