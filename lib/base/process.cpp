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

#include "i2-base.h"

using namespace icinga;

boost::once_flag Process::m_ThreadOnce;
boost::mutex Process::m_Mutex;
deque<Process::Ptr> Process::m_Tasks;

Process::Process(const vector<String>& arguments, const Dictionary::Ptr& extraEnvironment)
	: AsyncTask<Process, ProcessResult>(), m_Arguments(arguments), m_ExtraEnvironment(extraEnvironment)
{
	boost::call_once(&Process::Initialize, m_ThreadOnce);

#ifndef _WIN32
	m_FD = -1;
#endif /* _WIN32 */
}

vector<String> Process::SplitCommand(const Value& command)
{
	vector<String> args;

	if (command.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = command;
		Value arg;
		BOOST_FOREACH(tie(tuples::ignore, arg), dict) {
			args.push_back(arg);
		}

		return args;
	}

	// TODO: implement
#ifdef _WIN32
	args.push_back(command);
#else /* _WIN32 */
	args.push_back("sh");
	args.push_back("-c");
	args.push_back(command);
#endif
	return args;
}

void Process::Run(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Tasks.push_back(GetSelf());
	}

	NotifyWorker();
}
