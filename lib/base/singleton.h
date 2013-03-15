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

#ifndef SINGLETON_H
#define SINGLETON_H

namespace icinga
{

/**
 * A singleton.
 *
 * @ingroup base
 */
template<typename T>
class I2_BASE_API Singleton
{
public:
	static T *GetInstance(void)
	{
		/* FIXME: This relies on static initializers being atomic. */
		static boost::mutex mutex;
		boost::mutex::scoped_lock lock(mutex);

		if (!m_Instance)
			m_Instance = new T();

		return m_Instance;
	}
private:
	friend T *T::GetInstance(void);

	static T *m_Instance;
};

template<typename T>
T *Singleton<T>::m_Instance = NULL;

}

#endif /* SINGLETON_H */
