/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/createobjecthandler.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/httputility.hpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "remote/zone.hpp"
#include "base/configtype.hpp"
#include "base/defer.hpp"
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", CreateObjectHandler);

bool CreateObjectHandler::HandleRequest(
	const WaitGroup::Ptr& waitGroup,
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() != 4)
		return false;

	if (request.method() != http::verb::put)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(url->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
		return true;
	}

	auto requiredPermission ("objects/create/" + type->GetName());

	FilterUtility::CheckPermission(user, requiredPermission);

	String name = url->GetPath()[3];
	Array::Ptr templates = params->Get("templates");
	Dictionary::Ptr attrs = params->Get("attrs");

	/* Put created objects into the local zone if not explicitly defined.
	 * This allows additional zone members to sync the
	 * configuration at some later point.
	 */
	Zone::Ptr localZone = Zone::GetLocalZone();
	String localZoneName;

	if (localZone) {
		localZoneName = localZone->GetName();

		if (!attrs) {
			attrs = new Dictionary({
				{ "zone", localZoneName }
			});
		} else if (!attrs->Contains("zone")) {
			attrs->Set("zone", localZoneName);
		}
	}

	/* Sanity checks for unique groups array. */
	if (attrs->Contains("groups")) {
		Array::Ptr groups = attrs->Get("groups");

		if (groups)
			attrs->Set("groups", groups->Unique());
	}

	Dictionary::Ptr result1 = new Dictionary();
	String status;
	Array::Ptr errors = new Array();
	Array::Ptr diagnosticInformation = new Array();

	bool ignoreOnError = false;

	if (params->Contains("ignore_on_error"))
		ignoreOnError = HttpUtility::GetLastParameter(params, "ignore_on_error");

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	String config;

	bool verbose = false;

	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	ConfigObjectsSharedLock lock (std::try_to_lock);

	if (!lock) {
		HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading");
		return true;
	}

	std::shared_lock wgLock{*waitGroup, std::try_to_lock};
	if (!wgLock) {
		HttpUtility::SendJsonError(response, params, 503, "Shutting down.");
		return true;
	}

	/* Object creation can cause multiple errors and optionally diagnostic information.
	 * We can't use SendJsonError() here.
	 */
	try {
		config = ConfigObjectUtility::CreateObjectConfig(type, name, ignoreOnError, templates, attrs);
	} catch (const std::exception& ex) {
		errors->Add(DiagnosticInformation(ex, false));
		diagnosticInformation->Add(DiagnosticInformation(ex));

		if (verbose)
			result1->Set("diagnostic_information", diagnosticInformation);

		result1->Set("errors", errors);
		result1->Set("code", 500);
		result1->Set("status", "Object could not be created.");

		response.result(http::status::internal_server_error);
		HttpUtility::SendJsonBody(response, params, result);

		return true;
	}

	// Lock the object name of the given type to prevent from being created concurrently.
	ObjectNameLock objectNameLock(type, name);

	{
		auto permissions (user->GetPermissions());

		if (permissions) {
			std::unique_ptr<Expression> filters;

			{
				ObjectLock oLock (permissions);

				for (auto& item : permissions) {
					Function::Ptr filter;

					if (item.IsObjectType<Dictionary>()) {
						Dictionary::Ptr dict = item;
						filter = dict->Get("filter");

						if (Utility::Match(dict->Get("permission"), requiredPermission)) {
							if (!filter) {
								filters.reset();
								break;
							}

							std::vector<std::unique_ptr<Expression>> args;
							args.emplace_back(new GetScopeExpression(ScopeThis));

							std::unique_ptr<Expression> indexer = (std::unique_ptr<Expression>)new IndexerExpression(
								std::unique_ptr<Expression>(MakeLiteral(filter)),
								std::unique_ptr<Expression>(MakeLiteral("call"))
							);

							std::unique_ptr<Expression> fexpr = (std::unique_ptr<Expression>)new FunctionCallExpression(
								std::move(indexer), std::move(args)
							);

							if (filters) {
								filters = (std::unique_ptr<Expression>)new LogicalOrExpression(
									std::move(filters), std::move(fexpr)
								);
							} else {
								filters = std::move(fexpr);
							}
						}
					} else if (Utility::Match(item, requiredPermission)) {
						filters.reset();
						break;
					}
				}
			}

			if (filters) {
				try {
					auto expr (ConfigCompiler::CompileText("<api>", config, "", "_api"));

					ConfigItem::Ptr item;
					ConfigItem::m_OverrideRegistry = [&item](ConfigItem *ci) { item = ci; };

					Defer overrideRegistry ([]() {
						ConfigItem::m_OverrideRegistry = decltype(ConfigItem::m_OverrideRegistry)();
					});

					ActivationScope ascope;

					{
						ScriptFrame frame(true);
						expr->Evaluate(frame);
					}

					expr.reset();

					if (item) {
						auto obj (item->Commit(false));

						if (obj) {
							ScriptFrame frame (false, new Namespace());

							if (!FilterUtility::EvaluateFilter(frame, filters.get(), obj)) {
								BOOST_THROW_EXCEPTION(ScriptError(
									"Access denied to object '" + name + "' of type '" + type->GetName() + "'"
								));
							}
						}
					}
				} catch (const std::exception& ex) {
					result1->Set("errors", new Array({ ex.what() }));
					result1->Set("code", 500);
					result1->Set("status", "Object could not be created.");

					if (verbose)
						result1->Set("diagnostic_information", DiagnosticInformation(ex));

					response.result(http::status::internal_server_error);
					HttpUtility::SendJsonBody(response, params, result);

					return true;
				}
			}
		}
	}

	if (!ConfigObjectUtility::CreateObject(type, name, config, errors, diagnosticInformation)) {
		result1->Set("errors", errors);
		result1->Set("code", 500);
		result1->Set("status", "Object could not be created.");

		if (verbose)
			result1->Set("diagnostic_information", diagnosticInformation);

		response.result(http::status::internal_server_error);
		HttpUtility::SendJsonBody(response, params, result);

		return true;
	}

	auto *ctype = dynamic_cast<ConfigType *>(type.get());
	ConfigObject::Ptr obj = ctype->GetObject(name);

	result1->Set("code", 200);

	if (obj)
		result1->Set("status", "Object was created");
	else if (!obj && ignoreOnError)
		result1->Set("status", "Object was not created but 'ignore_on_error' was set to true");

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
