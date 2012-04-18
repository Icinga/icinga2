#ifndef EVENT_H
#define EVENT_H

namespace icinga
{

struct I2_BASE_API EventArgs
{
	Object::Ptr Source;
};

template<class TArgs>
class Event
{
public:
	typedef function<int (const TArgs&)> DelegateType;

private:
	list<DelegateType> m_Delegates;

public:
	void Hook(const DelegateType& delegate)
	{
		m_Delegates.push_front(delegate);
	}

	void Unhook(const DelegateType& delegate)
	{
		m_Delegates.remove(delegate);
	}

	Event<TArgs>& operator +=(const DelegateType& rhs)
	{
		Hook(rhs);
		return *this;
	}

	Event<TArgs>& operator -=(const DelegateType& rhs)
	{
		Unhook(rhs);
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
