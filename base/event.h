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
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef EVENT_H
#define EVENT_H

namespace icinga
{

/**
 * Base class for event arguments.
 */
struct I2_BASE_API EventArgs
{
	Object::Ptr Source; /**< The source of the event. */
};

/**
 * An observable event.
 */
template<class TArgs>
class Event
{
public:
	typedef function<int (const TArgs&)> ObserverType;

private:
	vector<ObserverType> m_Observers;

public:
	/**
	 * Adds an observer to this event.
	 *
	 * @param rhs The delegate.
	 */
	Event<TArgs>& operator +=(const ObserverType& rhs)
	{
		m_Observers.push_back(rhs);
		return *this;
	}

	/**
	 * Removes an observer from this event.
	 *
	 * @param rhs The delegate.
	 */
	Event<TArgs>& operator -=(const ObserverType& rhs)
	{
		m_Observers.erase(rhs);
		return *this;
	}

	/**
	 * Invokes each observer function that is registered for this event. Any
	 * observer function which returns -1 is removed.
	 *
	 * @param args Event arguments.
	 */
	void operator()(const TArgs& args)
	{
		typename vector<ObserverType>::iterator i;

		for (i = m_Observers.begin(); i != m_Observers.end(); ) {
			int result = (*i)(args);

			if (result == -1)
				i = m_Observers.erase(i);
			else
				i++;
		}
	}
};

}

#endif /* EVENT_H */
