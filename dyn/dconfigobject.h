#ifndef DCONFIGOBJECT_H
#define DCONFIGOBJECT_H

namespace icinga
{

class DConfigObject : public Object {
public:
	typedef shared_ptr<DConfigObject> Ptr;
	typedef weak_ptr<DConfigObject> WeakPtr;

	DConfigObject(string type, string name, long debuginfo);

	string GetType(void) const;
	string GetName(void) const;

	vector<string> GetParents(void) const;
	void AddParent(string parent);

	ExpressionList::Ptr GetExpressionList(void) const;
	void SetExpressionList(const ExpressionList::Ptr& exprl);

	Dictionary::Ptr CalculateProperties(void) const;

	static ObjectSet<DConfigObject::Ptr>::Ptr GetAllObjects(void);
	static ObjectMap<pair<string, string>, DConfigObject::Ptr>::Ptr GetObjectsByTypeAndName(void);
	static void AddObject(DConfigObject::Ptr object);
	static void RemoveObject(DConfigObject::Ptr object);
	static DConfigObject::Ptr GetObject(string type, string name);

private:
	string m_Type;
	string m_Name;
	long m_DebugInfo;
	vector<string> m_Parents;
	ExpressionList::Ptr m_ExpressionList;

	static bool GetTypeAndName(const DConfigObject::Ptr& object, pair<string, string> *key);
};


}

#endif /* DCONFIGOBJECT_H */
