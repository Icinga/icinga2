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
	vector<DelegateType> m_Delegates;

public:
	/**
	 * operator +=
	 *
	 * Adds a delegate to this event.
	 *
	 * @param rhs The delegate.
	 */
	Event<TArgs>& operator +=(const DelegateType& rhs)
	{
		m_Delegates.push_back(rhs);
		return *this;
	}

	/**
	 * operator -=
	 *
	 * Removes a delegate from this event.
	 *
	 * @param rhs The delegate.
	 */
	Event<TArgs>& operator -=(const DelegateType& rhs)
	{
		m_Delegates.erase(rhs);
		return *this;
	}

	/**
	 * operator ()
	 *
	 * Invokes each delegate that is registered for this event. Any delegates
	 * which return -1 are removed.
	 *
	 * @param args Event arguments.
	 */
	void operator()(const TArgs& args)
	{
		typename vector<DelegateType>::iterator i;

		for (i = m_Delegates.begin(); i != m_Delegates.end(); ) {
			int result = (*i)(args);

			if (result == -1)
				i = m_Delegates.erase(i);
			else
				i++;
		}
	}
};

}

#endif /* EVENT_H */
