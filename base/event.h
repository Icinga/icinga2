#ifndef EVENT_H
#define EVENT_H

namespace icinga
{

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

			int result = (*prev)(args);

			if (result == -1)
				m_Delegates.erase(prev);
		}
	}
};

}

#endif /* EVENT_H */
