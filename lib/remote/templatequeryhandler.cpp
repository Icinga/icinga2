/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

bool TemplateQueryHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() < 3 || request.RequestUrl->GetPath().size() > 4)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "templates/query/" + type->GetName();
	qd.Provider = new TemplateTargetProvider();

	params->Set("type", type->GetName());

	if (request.RequestUrl->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, request.RequestUrl->GetPath()[3]);
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

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
