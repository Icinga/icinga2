#ifndef EVENT_H
#define EVENT_H

namespace icinga
{

struct I2_BASE_API EventArgs : public Object
{
	typedef shared_ptr<EventArgs> Ptr;
	typedef weak_ptr<EventArgs> WeakPtr;

	Object::Ptr Source;
};

template<class TArgs>
class event
{
public:
	typedef function<int (TArgs)> DelegateType;

private:
	list<DelegateType> m_Delegates;

public:
	void hook(const DelegateType& delegate)
	{
		m_Delegates.push_front(delegate);
	}

	void unhook(const DelegateType& delegate)
	{
		m_Delegates.remove(delegate);
	}

	event<TArgs>& operator +=(const DelegateType& rhs)
	{
		hook(rhs);
		return *this;
	}

	event<TArgs>& operator -=(const DelegateType& rhs)
	{
		unhook(rhs);
		return *this;
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
