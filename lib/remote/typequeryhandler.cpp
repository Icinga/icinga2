/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/typequeryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/configtype.hpp"
#include "base/scriptglobal.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/types", TypeQueryHandler);

class TypeTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(TypeTargetProvider);

	void FindTargets(const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		for (const Type::Ptr& target : Type::GetAllTypes()) {
			addTarget(target);
		}
	}

	Value GetTargetByName(const String& type, const String& name) const override
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

	String GetPluralName(const String& type) const override
	{
		return "types";
	}
};

bool TypeQueryHandler::HandleRequest(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

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

	ArrayData results;

	for (const Type::Ptr& obj : objs) {
		Dictionary::Ptr result1 = new Dictionary();
		results.push_back(result1);

		Dictionary::Ptr resultAttrs = new Dictionary();
		result1->Set("name", obj->GetName());
		result1->Set("plural_name", obj->GetPluralName());
		if (obj->GetBaseType())
			result1->Set("base", obj->GetBaseType()->GetName());
		result1->Set("abstract", obj->IsAbstract());
		result1->Set("fields", resultAttrs);

		Dictionary::Ptr prototype = dynamic_pointer_cast<Dictionary>(obj->GetPrototype());
		Array::Ptr prototypeKeys = new Array();
		result1->Set("prototype_keys", prototypeKeys);

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
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
