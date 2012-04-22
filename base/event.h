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
	Event<TArgs>& operator +=(const DelegateType& rhs)
	{
		m_Delegates.push_back(rhs);
		return *this;
	}

	Event<TArgs>& operator -=(const DelegateType& rhs)
	{
		m_Delegates.erase(rhs);
		return *this;
	}

	void operator()(const TArgs& args)
	{
		typename vector<DelegateType>::iterator prev, i;

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
