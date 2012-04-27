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
