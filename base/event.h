/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

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
