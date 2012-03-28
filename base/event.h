#ifndef I2_EVENT_H
#define I2_EVENT_H

namespace icinga
{

using std::list;

struct EventArgs : public Object
{
	typedef shared_ptr<EventArgs> RefType;
	typedef weak_ptr<EventArgs> WeakRefType;

	Object::RefType Source;
};

template<class TArgs>
class event
{
public:
	typedef function<int (TArgs)> DelegateType;

private:
	list<DelegateType> m_Delegates;

public:
	void bind(const DelegateType& delegate)
	{
		m_Delegates.push_front(delegate);
	}

	void unbind(const DelegateType& delegate)
	{
		m_Delegates.remove(delegate);
	}

	void operator()(const TArgs& args)
	{
		typename list<DelegateType>::iterator prev, i;

		for (i = m_Delegates.begin(); i != m_Delegates.end(); ) {
			prev = i;
			i++;

			(*prev)(args);
		}
	}
};

}

#endif /* I2_EVENT_H */
