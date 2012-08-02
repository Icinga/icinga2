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

#ifndef SCRIPTTASK_H
#define SCRIPTTASK_H

namespace icinga
{

class I2_BASE_API ScriptTask : public AsyncTask<ScriptTask, Value>
{
public:
	typedef shared_ptr<ScriptTask> Ptr;
	typedef weak_ptr<ScriptTask> WeakPtr;

	ScriptTask(const ScriptFunction::Ptr& function, const vector<Value>& arguments);

protected:
	virtual void Run(void);

private:
	ScriptFunction::Ptr m_Function;
	vector<Value> m_Arguments;
};

}

#endif /* SCRIPTTASK_H */
