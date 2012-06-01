#ifndef DCONFIGOBJECT_H
#define DCONFIGOBJECT_H

namespace icinga
{

class DConfigObject : public Object {
public:
	typedef shared_ptr<DConfigObject> Ptr;
	typedef weak_ptr<DConfigObject> WeakPtr;

	vector<string> GetParents(void) const;
	void AddParent(string parent);

	ExpressionList::Ptr GetExpressionList(void) const;
	void SetExpressionList(const ExpressionList::Ptr& exprl);

private:
	string m_Type;
	string m_Name;
	vector<string> m_Parents;
	ExpressionList::Ptr m_ExpressionList;

};


}

#endif /* DCONFIGOBJECT_H */
