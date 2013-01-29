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

#ifndef COMPATCOMPONENT_H
#define COMPATCOMPONENT_H

namespace icinga
{

/**
 * @ingroup compat
 */
class CompatComponent : public IComponent
{
public:
	virtual void Start(void);
	virtual void Stop(void);

private:
#ifndef _WIN32
	thread m_CommandThread;

	void CommandPipeThread(const String& commandPath);
	void ProcessCommand(const String& command);
#endif /* _WIN32 */

	Timer::Ptr m_StatusTimer;

	String GetStatusPath(void) const;
	String GetObjectsPath(void) const;
	String GetCommandPath(void) const;

	void DumpDowntimes(ofstream& fp, const DynamicObject::Ptr& owner);
	void DumpComments(ofstream& fp, const DynamicObject::Ptr& owner);
	void DumpHostStatus(ofstream& fp, const Host::Ptr& host);
	void DumpHostObject(ofstream& fp, const Host::Ptr& host);

	template<typename T>
	void DumpNameList(ofstream& fp, const T& list)
	{
		typename T::const_iterator it;
		bool first = true;
		for (it = list.begin(); it != list.end(); it++) {
			if (!first)
				fp << ",";
			else
				first = false;

			fp << (*it)->GetName();
		}
	}

	template<typename T>
	void DumpStringList(ofstream& fp, const T& list)
	{
		typename T::const_iterator it;
		bool first = true;
		for (it = list.begin(); it != list.end(); it++) {
			if (!first)
				fp << ",";
			else
				first = false;

			fp << *it;
		}
	}

	void DumpServiceStatus(ofstream& fp, const Service::Ptr& service);
	void DumpServiceObject(ofstream& fp, const Service::Ptr& service);

	void StatusTimerHandler(void);

	static const String DefaultStatusPath;
	static const String DefaultObjectsPath;
	static const String DefaultCommandPath;
};

}

#endif /* COMPATCOMPONENT_H */
