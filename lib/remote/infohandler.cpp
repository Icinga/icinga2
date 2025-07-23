/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/infohandler.hpp"
#include "remote/httputility.hpp"
#include "base/application.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/", InfoHandler);

bool InfoHandler::HandleRequest(
	const WaitGroup::Ptr&,
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() > 2)
		return false;

	if (request.method() != http::verb::get)
		return false;

	if (url->GetPath().empty()) {
		response.result(http::status::found);
		response.set(http::field::location, "/v1");
		return true;
	}

	if (url->GetPath()[0] != "v1" || url->GetPath().size() != 1)
		return false;

	response.result(http::status::ok);

	std::vector<String> permInfo;
	Array::Ptr permissions = user->GetPermissions();

	if (permissions) {
		ObjectLock olock(permissions);
		for (const Value& permission : permissions) {
			String name;
			bool hasFilter = false;
			if (permission.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dpermission = permission;
				name = dpermission->Get("permission");
				hasFilter = dpermission->Contains("filter");
			} else
				name = permission;

			if (hasFilter)
				name += " (filtered)";

			permInfo.emplace_back(std::move(name));
		}
	}

	if (request[http::field::accept] == "application/json") {
		Dictionary::Ptr result1 = new Dictionary({
			{ "user", user->GetName() },
			{ "permissions", Array::FromVector(permInfo) },
			{ "version", Application::GetAppVersion() },
			{ "info", "More information about API requests is available in the documentation at https://icinga.com/docs/icinga2/latest/" }
		});

		Dictionary::Ptr result = new Dictionary({
			{ "results", new Array({ result1 }) }
		});

		HttpUtility::SendJsonBody(response, params, result);
	} else {
		response.set(http::field::content_type, "text/html");

		auto & body = response.body();
		body << "<html><head><title>Icinga 2</title></head><h1>Hello from Icinga 2 (Version: "
			<< Application::GetAppVersion() << ")!</h1>"
			<< "<p>You are authenticated as <b>" << user->GetName() << "</b>. ";

		if (!permInfo.empty()) {
			body << "Your user has the following permissions:</p> <ul>";

			for (const String& perm : permInfo) {
				body << "<li>" << perm << "</li>";
			}

			body << "</ul>";
		} else
			body << "Your user does not have any permissions.</p>";

		body << R"(<p>More information about API requests is available in the <a href="https://icinga.com/docs/icinga2/latest/" target="_blank">documentation</a>.</p></html>)";
	}

	return true;
}
