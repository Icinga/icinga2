/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/objectqueryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/serializer.hpp"
#include "base/dependencygraph.hpp"
#include "base/configtype.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", ObjectQueryHandler);

Dictionary::Ptr ObjectQueryHandler::SerializeObjectAttrs(const Object::Ptr& object,
	const String& attrPrefix, const Array::Ptr& attrs, bool isJoin, bool allAttrs)
{
	Type::Ptr type = object->GetReflectionType();

	std::vector<int> fids;

	if (isJoin && attrs) {
		ObjectLock olock(attrs);
		for (const String& attr : attrs) {
			if (attr == attrPrefix) {
				allAttrs = true;
				break;
			}
		}
	}

	if (!isJoin && (!attrs || attrs->GetLength() == 0))
		allAttrs = true;

	if (allAttrs) {
		for (int fid = 0; fid < type->GetFieldCount(); fid++) {
			fids.push_back(fid);
		}
	} else if (attrs) {
		ObjectLock olock(attrs);
		for (const String& attr : attrs) {
			String userAttr;

			if (isJoin) {
				String::SizeType dpos = attr.FindFirstOf(".");
				if (dpos == String::NPos)
					continue;

				String userJoinAttr = attr.SubStr(0, dpos);
				if (userJoinAttr != attrPrefix)
					continue;

				userAttr = attr.SubStr(dpos + 1);
			} else
				userAttr = attr;

			int fid = type->GetFieldId(userAttr);

			if (fid < 0)
				BOOST_THROW_EXCEPTION(ScriptError("Invalid field specified: " + userAttr));

			fids.push_back(fid);
		}
	}

	DictionaryData resultAttrs;
	resultAttrs.reserve(fids.size());

	for (int fid : fids) {
		Field field = type->GetFieldInfo(fid);

		Value val = object->GetField(fid);

		/* hide attributes which shouldn't be user-visible */
		if (field.Attributes & FANoUserView)
			continue;

		/* hide internal navigation fields */
		if (field.Attributes & FANavigation && !(field.Attributes & (FAConfig | FAState)))
			continue;

		Value sval = Serialize(val, FAConfig | FAState);
		resultAttrs.emplace_back(field.Name, sval);
	}

	return new Dictionary(std::move(resultAttrs));
}

bool ObjectQueryHandler::HandleRequest(
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
	qd.Permission = "objects/query/" + type->GetName();

	Array::Ptr uattrs, ujoins, umetas;

	try {
		uattrs = params->Get("attrs");
	} catch (const std::exception&) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'attrs' attribute specified. Array type is required.");
		return true;
	}

	try {
		ujoins = params->Get("joins");
	} catch (const std::exception&) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'joins' attribute specified. Array type is required.");
		return true;
	}

	try {
		umetas = params->Get("meta");
	} catch (const std::exception&) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'meta' attribute specified. Array type is required.");
		return true;
	}

	bool allJoins = HttpUtility::GetLastParameter(params, "all_joins");

	params->Set("type", type->GetName());

	if (url->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, url->GetPath()[3]);
	}

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
	results.reserve(objs.size());

	std::set<String> joinAttrs;
	std::set<String> userJoinAttrs;

	if (ujoins) {
		ObjectLock olock(ujoins);
		for (const String& ujoin : ujoins) {
			userJoinAttrs.insert(ujoin.SubStr(0, ujoin.FindFirstOf(".")));
		}
	}

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if (!(field.Attributes & FANavigation))
			continue;

		if (!allJoins && userJoinAttrs.find(field.NavigationName) == userJoinAttrs.end())
			continue;

		joinAttrs.insert(field.Name);
	}

	for (const ConfigObject::Ptr& obj : objs) {
		DictionaryData result1{
			{ "name", obj->GetName() },
			{ "type", obj->GetReflectionType()->GetName() }
		};

		DictionaryData metaAttrs;

		if (umetas) {
			ObjectLock olock(umetas);
			for (const String& meta : umetas) {
				if (meta == "used_by") {
					Array::Ptr used_by = new Array();
					metaAttrs.emplace_back("used_by", used_by);

					for (const Object::Ptr& pobj : DependencyGraph::GetParents((obj)))
					{
						ConfigObject::Ptr configObj = dynamic_pointer_cast<ConfigObject>(pobj);

						if (!configObj)
							continue;

						used_by->Add(new Dictionary({
							{ "type", configObj->GetReflectionType()->GetName() },
							{ "name", configObj->GetName() }
						}));
					}
				} else if (meta == "location") {
					metaAttrs.emplace_back("location", obj->GetSourceLocation());
				} else {
					HttpUtility::SendJsonError(response, params, 400, "Invalid field specified for meta: " + meta);
					return true;
				}
			}
		}

		result1.emplace_back("meta", new Dictionary(std::move(metaAttrs)));

		try {
			result1.emplace_back("attrs", SerializeObjectAttrs(obj, String(), uattrs, false, false));
		} catch (const ScriptError& ex) {
			HttpUtility::SendJsonError(response, params, 400, ex.what());
			return true;
		}

		DictionaryData joins;

		for (const String& joinAttr : joinAttrs) {
			Object::Ptr joinedObj;
			int fid = type->GetFieldId(joinAttr);

			if (fid < 0) {
				HttpUtility::SendJsonError(response, params, 400, "Invalid field specified for join: " + joinAttr);
				return true;
			}

			Field field = type->GetFieldInfo(fid);

			if (!(field.Attributes & FANavigation)) {
				HttpUtility::SendJsonError(response, params, 400, "Not a joinable field: " + joinAttr);
				return true;
			}

			joinedObj = obj->NavigateField(fid);

			if (!joinedObj)
				continue;

			String prefix = field.NavigationName;

			try {
				joins.emplace_back(prefix, SerializeObjectAttrs(joinedObj, prefix, ujoins, true, allJoins));
			} catch (const ScriptError& ex) {
				HttpUtility::SendJsonError(response, params, 400, ex.what());
				return true;
			}
		}

		result1.emplace_back("joins", new Dictionary(std::move(joins)));

		results.push_back(new Dictionary(std::move(result1)));
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
