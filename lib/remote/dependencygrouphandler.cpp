/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base/scriptglobal.hpp"
#include "remote/dependencygrouphandler.hpp"
#include "remote/httputility.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects/dependencygroups", DependencyGroupHandler);

bool DependencyGroupHandler::HandleRequest(
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() != 3 || request.method() != http::verb::get) {
		return false;
	}

	if (!user->GetPermissions()->Contains("*")) { // Only superusers are allowed to access this endpoint.
		return false;
	}

	Namespace::Ptr internalNS = ScriptGlobal::Get("Internal");
	VERIFY(internalNS);

	if (!internalNS->Contains("DependencyGroupsSerializer")) {
		return false;
	}

	Function::Ptr dependencyGroupSerializer(internalNS->Get("DependencyGroupsSerializer"));
	Dictionary::Ptr result = new Dictionary{{"results", dependencyGroupSerializer->Invoke()}};
	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
