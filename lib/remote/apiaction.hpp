/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APIACTION_H
#define APIACTION_H

#include "remote/i2-remote.hpp"
#include "base/registry.hpp"
#include "base/value.hpp"
#include "base/dictionary.hpp"
#include "base/configobject.hpp"
#include <vector>
#include <boost/algorithm/string/replace.hpp>

namespace icinga
{

/**
 * An API action.
 *
 * @ingroup remote
 */
class ApiAction final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ApiAction);

	typedef std::function<Value(const ConfigObject::Ptr& target, const Dictionary::Ptr& params)> Callback;

	ApiAction(std::vector<String> registerTypes, Callback function);

	Value Invoke(const ConfigObject::Ptr& target, const Dictionary::Ptr& params);

	const std::vector<String>& GetTypes() const;

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
class ApiActionRegistry : public Registry<ApiActionRegistry, ApiAction::Ptr>
{
public:
	static ApiActionRegistry *GetInstance();
};

#define REGISTER_APIACTION(name, types, callback) \
	INITIALIZE_ONCE([]() { \
		String registerName = #name; \
		boost::algorithm::replace_all(registerName, "_", "-"); \
		std::vector<String> registerTypes; \
		String typeNames = types; \
		if (!typeNames.IsEmpty()) \
			registerTypes = typeNames.Split(";"); \
		ApiAction::Ptr action = new ApiAction(registerTypes, callback); \
		ApiActionRegistry::GetInstance()->Register(registerName, action); \
	})

}

#endif /* APIACTION_H */
