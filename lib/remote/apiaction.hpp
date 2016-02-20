/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef APIACTION_H
#define APIACTION_H

#include "remote/i2-remote.hpp"
#include "base/registry.hpp"
#include "base/value.hpp"
#include "base/dictionary.hpp"
#include "base/configobject.hpp"
#include <vector>
#include <boost/function.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace icinga
{

/**
 * An API action.
 *
 * @ingroup remote
 */
class I2_REMOTE_API ApiAction : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ApiAction);

	typedef boost::function<Value(const ConfigObject::Ptr& target, const Dictionary::Ptr& params)> Callback;

	ApiAction(const std::vector<String>& registerTypes, const Callback& function);

	Value Invoke(const ConfigObject::Ptr& target, const Dictionary::Ptr& params);

	const std::vector<String>& GetTypes(void) const;

	static ApiAction::Ptr GetByName(const String& name);
	static void Register(const String& name, const ApiAction::Ptr& action);
	static void Unregister(const String& name);

private:
	std::vector<String> m_Types;
	Callback m_Callback;
};

/**
 * A registry for API actions.
 *
 * @ingroup remote
 */
class I2_REMOTE_API ApiActionRegistry : public Registry<ApiActionRegistry, ApiAction::Ptr>
{
public:
	static ApiActionRegistry *GetInstance(void);
};

#define REGISTER_APIACTION(name, types, callback) \
	namespace { namespace UNIQUE_NAME(apia) { namespace apia ## name { \
		void RegisterAction(void) \
		{ \
			String registerName = #name; \
			boost::algorithm::replace_all(registerName, "_", "-"); \
			std::vector<String> registerTypes; \
			String typeNames = types; \
			if (!typeNames.IsEmpty()) \
				boost::algorithm::split(registerTypes, typeNames, boost::is_any_of(";")); \
			ApiAction::Ptr action = new ApiAction(registerTypes, callback); \
			ApiActionRegistry::GetInstance()->Register(registerName, action); \
		} \
		INITIALIZE_ONCE(RegisterAction); \
	} } }

}

#endif /* APIACTION_H */
