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

#ifndef CONFIGOBJECTADAPTER_H
#define CONFIGOBJECTADAPTER_H

namespace icinga
{

class I2_CIB_API ConfigObjectAdapter
{
public:
	ConfigObjectAdapter(const ConfigObject::Ptr& configObject)
		: m_ConfigObject(configObject)
	{ }

	string GetType(void) const;
	string GetName(void) const;

	bool IsLocal(void) const;

	ConfigObject::Ptr GetConfigObject() const;

	template<typename T>
	bool GetProperty(const string& key, T *value) const
	{
		return GetConfigObject()->GetProperty(key, value);
	}

	template<typename T>
	void SetTag(const string& key, const T& value)
	{
		GetConfigObject()->SetTag(key, value);
	}

	template<typename T>
	bool GetTag(const string& key, T *value) const
	{
		return GetConfigObject()->GetTag(key, value);
	}

	void RemoveTag(const string& key);

	ScriptTask::Ptr InvokeHook(const string& hook,
	    const vector<Variant>& arguments, ScriptTask::CompletionCallback callback);

private:
	ConfigObject::Ptr m_ConfigObject;
};

}

#endif /* CONFIGOBJECTADAPTER_H */
