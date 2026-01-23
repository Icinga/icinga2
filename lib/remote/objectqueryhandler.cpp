/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/objectqueryhandler.hpp"
#include "base/generator.hpp"
#include "base/json.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/serializer.hpp"
#include "base/dependencygraph.hpp"
#include "base/configtype.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>
#include <unordered_map>
#include <memory>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", ObjectQueryHandler);

Dictionary::Ptr ObjectQueryHandler::SerializeObjectAttrs(const Object::Ptr& object,
	const String& attrPrefix, const Array::Ptr& attrs, bool isJoin, bool allAttrs)
{
	Type::Ptr type = object->GetReflectionType();

	std::vector<int> fids;

	if (isJoin && attrs) {
		ObjectLock olock(attrs);
		for (String attr : attrs) {
			if (attr == attrPrefix) {
				allAttrs = true;
				break;
			}
		}
	}

	if (!isJoin && !attrs)
		allAttrs = true;

	if (allAttrs) {
		for (int fid = 0; fid < type->GetFieldCount(); fid++) {
			fids.push_back(fid);
		}
	} else if (attrs) {
		ObjectLock olock(attrs);
		for (String attr : attrs) {
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

	bool includeUsedBy = false;
	bool includeLocation = false;
	if (umetas) {
		ObjectLock olock(umetas);
		for (String meta : umetas) {
			if (meta == "used_by") {
				includeUsedBy = true;
			} else if (meta == "location") {
				includeLocation = true;
			} else {
				HttpUtility::SendJsonError(response, params, 400, "Invalid field specified for meta: " + meta);
				return true;
			}
		}
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

	std::set<int> joinAttrs;
	std::set<String> userJoinAttrs;

	if (ujoins) {
		ObjectLock olock(ujoins);
		for (String ujoin : ujoins) {
			userJoinAttrs.insert(ujoin.SubStr(0, ujoin.FindFirstOf(".")));
		}
	}

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if (!(field.Attributes & FANavigation))
			continue;

		if (!allJoins && userJoinAttrs.find(field.NavigationName) == userJoinAttrs.end())
			continue;

		joinAttrs.insert(fid);
	}

	std::unordered_map<Type*, std::pair<bool, std::unique_ptr<Expression>>> typePermissions;
	std::unordered_map<Object*, bool> objectAccessAllowed;

	auto it = objs.begin();
	auto generatorFunc = [&]() -> std::optional<Value> {
		if (it == objs.end()) {
			return std::nullopt;
		}

		ConfigObject::Ptr obj = *it;
		++it;

		DictionaryData result1{
			{ "name", obj->GetName() },
			{ "type", obj->GetReflectionType()->GetName() }
		};

		DictionaryData metaAttrs;
		if (includeUsedBy) {
			Array::Ptr used_by = new Array();
			metaAttrs.emplace_back("used_by", used_by);

			for (auto& configObj : DependencyGraph::GetChildren(obj)) {
				used_by->Add(new Dictionary({
					{"type", configObj->GetReflectionType()->GetName()},
					{"name", configObj->GetName()}
				}));
			}
		}

		if (includeLocation) {
			metaAttrs.emplace_back("location", obj->GetSourceLocation());
		}

		result1.emplace_back("meta", new Dictionary(std::move(metaAttrs)));

		try {
			result1.emplace_back("attrs", SerializeObjectAttrs(obj, String(), uattrs, false, false));
		} catch (const ScriptError& ex) {
			return new Dictionary{
				{"type", type->GetName()},
				{"name", obj->GetName()},
				{"code", 400},
				{"status", ex.what()}
			};
		}

		DictionaryData joins;

		for (auto joinAttr : joinAttrs) {
			Object::Ptr joinedObj;
			Field field = type->GetFieldInfo(joinAttr);

			joinedObj = obj->NavigateField(joinAttr);

			if (!joinedObj)
				continue;

			Type::Ptr reflectionType = joinedObj->GetReflectionType();
			auto it = typePermissions.find(reflectionType.get());
			bool granted;

			if (it == typePermissions.end()) {
				String permission = "objects/query/" + reflectionType->GetName();

				std::unique_ptr<Expression> permissionFilter;
				granted = FilterUtility::HasPermission(user, permission, &permissionFilter);

				it = typePermissions.insert({reflectionType.get(), std::make_pair(granted, std::move(permissionFilter))}).first;
			}

			granted = it->second.first;
			const std::unique_ptr<Expression>& permissionFilter = it->second.second;

			if (!granted) {
				// Not authorized
				continue;
			}

			auto relation = objectAccessAllowed.find(joinedObj.get());
			bool accessAllowed;

			if (relation == objectAccessAllowed.end()) {
				ScriptFrame permissionFrame(false, new Namespace());

				try {
					accessAllowed = FilterUtility::EvaluateFilter(permissionFrame, permissionFilter.get(), joinedObj);
				} catch (const ScriptError& err) {
					accessAllowed = false;
				}

				objectAccessAllowed.insert({joinedObj.get(), accessAllowed});
			} else {
				accessAllowed = relation->second;
			}

			if (!accessAllowed) {
				// Access denied
				continue;
			}

			String prefix = field.NavigationName;

			try {
				joins.emplace_back(prefix, SerializeObjectAttrs(joinedObj, prefix, ujoins, true, allJoins));
			} catch (const ScriptError& ex) {
				return new Dictionary{
					{"type", type->GetName()},
					{"name", obj->GetName()},
					{"code", 400},
					{"status", ex.what()}
				};
			}
		}

		result1.emplace_back("joins", new Dictionary(std::move(joins)));

		return new Dictionary{std::move(result1)};
	};

	response.result(http::status::ok);
	response.set(http::field::content_type, "application/json");
	response.StartStreaming(false);

	Dictionary::Ptr results = new Dictionary{{"results", new ValueGenerator{generatorFunc}}};
	results->Freeze();

	bool pretty = HttpUtility::GetLastParameter(params, "pretty");
	response.GetJsonEncoder(pretty).Encode(results, &yc);

	return true;
}
