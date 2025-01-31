/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base/scriptglobal.hpp"
#include "remote/dependencygrouphandler.hpp"
#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/debug/dependencygroups", DependencyGroupHandler);

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

	FilterUtility::CheckPermission(user, "debug");

	Namespace::Ptr internalNS = ScriptGlobal::Get("Internal");
	VERIFY(internalNS);

	if (!internalNS->Contains("DependencyGroupsSerializer")) {
		return false;
	}

	Function::Ptr dependencyGroupsSerializer(internalNS->Get("DependencyGroupsSerializer"));
	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, new Dictionary{{"results", dependencyGroupsSerializer->Invoke()}});

	return true;
}
