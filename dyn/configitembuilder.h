#ifndef CONFIGITEMBUILDER_H
#define CONFIGITEMBUILDER_H

namespace icinga
{

class I2_DYN_API ConfigItemBuilder : public Object
{
public:
	typedef shared_ptr<ConfigItemBuilder> Ptr;
	typedef weak_ptr<ConfigItemBuilder> WeakPtr;

	ConfigItemBuilder(void);
	ConfigItemBuilder(const DebugInfo& debugInfo);

	void SetType(const string& type);
	void SetName(const string& name);
	void SetLocal(bool local);
	void SetAbstract(bool abstract);

	void AddParent(const string& parent);

	void AddExpression(const Expression& expr);
	void AddExpression(const string& key, ExpressionOperator op, const Variant& value);
	void AddExpressionList(const ExpressionList::Ptr& exprl);

	ConfigItem::Ptr Compile(void);

private:
	string m_Type;
	string m_Name;
	bool m_Local;
	bool m_Abstract;
	vector<string> m_Parents;
	ExpressionList::Ptr m_ExpressionList;
	DebugInfo m_DebugInfo;
};

}

#endif /* CONFIGITEMBUILDER */
