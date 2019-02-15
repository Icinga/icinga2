/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/templatequeryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "config/configitem.hpp"
#include "base/configtype.hpp"
#include "base/scriptglobal.hpp"
#include "base/logger.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/templates", TemplateQueryHandler);

class TemplateTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(TemplateTargetProvider);

	static Dictionary::Ptr GetTargetForTemplate(const ConfigItem::Ptr& item)
	{
		DebugInfo di = item->GetDebugInfo();

		return new Dictionary({
			{ "name", item->GetName() },
			{ "type", item->GetType()->GetName() },
			{ "location", new Dictionary({
				{ "path", di.Path },
				{ "first_line", di.FirstLine },
				{ "first_column", di.FirstColumn },
				{ "last_line", di.LastLine },
				{ "last_column", di.LastColumn }
			}) }
		});
	}

	void FindTargets(const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		Type::Ptr ptype = Type::GetByName(type);

		for (const ConfigItem::Ptr& item : ConfigItem::GetItems(ptype)) {
			if (item->IsAbstract())
				addTarget(GetTargetForTemplate(item));
		}
	}

	Value GetTargetByName(const String& type, const String& name) const override
	{
		Type::Ptr ptype = Type::GetByName(type);

		ConfigItem::Ptr item = ConfigItem::GetByTypeAndName(ptype, name);

		if (!item || !item->IsAbstract())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Template does not exist."));

		return GetTargetForTemplate(item);
	}

	bool IsValidType(const String& type) const override
	{
		Type::Ptr ptype = Type::GetByName(type);

		if (!ptype)
			return false;

		return ConfigObject::TypeInstance->IsAssignableFrom(ptype);
	}

	String GetPluralName(const String& type) const override
	{
		return Type::GetByName(type)->GetPluralName();
	}
};

bool TemplateQueryHandler::HandleRequest(
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context& yc,
	bool& hasStartedStreaming
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() < 3 || url->GetPath().size() > 4)
		return false;

	if (request.method() != http::verb::get)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(url->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "templates/query/" + type->GetName();
	qd.Provider = new TemplateTargetProvider();

	params->Set("type", type->GetName());

	if (url->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, url->GetPath()[3]);
	}

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user, "tmpl");
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 404,
			"No templates found.",
			DiagnosticInformation(ex));
		return true;
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(objs)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
