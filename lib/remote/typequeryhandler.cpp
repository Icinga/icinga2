// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/typequeryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/generator.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/types", TypeQueryHandler);

class TypeTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(TypeTargetProvider);

	void FindTargets([[maybe_unused]] const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		for (const Type::Ptr& target : Type::GetAllTypes()) {
			addTarget(target);
		}
	}

	Value GetTargetByName([[maybe_unused]] const String& type, const String& name) const override
	{
		Type::Ptr ptype = Type::GetByName(name);

		if (!ptype)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Type does not exist."));

		return ptype;
	}

	bool IsValidType(const String& type) const override
	{
		return type == "Type";
	}

	String GetPluralName([[maybe_unused]] const String& type) const override
	{
		return "types";
	}
};

bool TypeQueryHandler::HandleRequest(
	const WaitGroup::Ptr&,
	const HttpApiRequest& request,
	HttpApiResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() > 3)
		return false;

	if (request.method() != http::verb::get)
		return false;

	QueryDescription qd;
	qd.Types.insert("Type");
	qd.Permission = "types";
	qd.Provider = new TypeTargetProvider();

	if (params->Contains("type"))
		params->Set("name", params->Get("type"));

	params->Set("type", "Type");

	if (url->GetPath().size() >= 3)
		params->Set("name", url->GetPath()[2]);

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 404,
			"No objects found.",
			DiagnosticInformation(ex));
		return true;
	}

	auto generatorFunc = [](const Type::Ptr& obj) -> Value {
		Dictionary::Ptr result = new Dictionary();
		Dictionary::Ptr resultAttrs = new Dictionary();
		result->Set("name", obj->GetName());
		result->Set("plural_name", obj->GetPluralName());
		if (obj->GetBaseType())
			result->Set("base", obj->GetBaseType()->GetName());
		result->Set("abstract", obj->IsAbstract());
		result->Set("fields", resultAttrs);

		Dictionary::Ptr prototype = dynamic_pointer_cast<Dictionary>(obj->GetPrototype());
		Array::Ptr prototypeKeys = new Array();
		result->Set("prototype_keys", prototypeKeys);

		if (prototype) {
			ObjectLock olock(prototype);
			for (const Dictionary::Pair& kv : prototype) {
				prototypeKeys->Add(kv.first);
			}
		}

		int baseFieldCount = 0;

		if (obj->GetBaseType())
			baseFieldCount = obj->GetBaseType()->GetFieldCount();

		for (int fid = baseFieldCount; fid < obj->GetFieldCount(); fid++) {
			Field field = obj->GetFieldInfo(fid);

			Dictionary::Ptr fieldInfo = new Dictionary();
			resultAttrs->Set(field.Name, fieldInfo);

			fieldInfo->Set("id", fid);
			fieldInfo->Set("type", field.TypeName);
			if (field.RefTypeName)
				fieldInfo->Set("ref_type", field.RefTypeName);
			if (field.Attributes & FANavigation)
				fieldInfo->Set("navigation_name", field.NavigationName);
			fieldInfo->Set("array_rank", field.ArrayRank);

			fieldInfo->Set("attributes", new Dictionary({
				{ "config", static_cast<bool>(field.Attributes & FAConfig) },
				{ "state", static_cast<bool>(field.Attributes & FAState) },
				{ "required", static_cast<bool>(field.Attributes & FARequired) },
				{ "navigation", static_cast<bool>(field.Attributes & FANavigation) },
				{ "no_user_modify", static_cast<bool>(field.Attributes & FANoUserModify) },
				{ "no_user_view", static_cast<bool>(field.Attributes & FANoUserView) },
				{ "deprecated", static_cast<bool>(field.Attributes & FADeprecated) }
			}));
		}
		return result;
	};

	Dictionary::Ptr result = new Dictionary{{"results", new ValueGenerator{objs, generatorFunc}}};
	result->Freeze();

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result, yc);

	return true;
}
