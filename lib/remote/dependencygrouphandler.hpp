/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCYGROUPHANDLER_H
#define DEPENDENCYGROUPHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga {

class DependencyGroupHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(DependencyGroupHandler);

	bool HandleRequest(
		AsioTlsStream& stream,
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params,
		boost::asio::yield_context& yc,
		HttpServerConnection& server
	) override;
};

}

#endif /* DEPENDENCYGROUPHANDLER_H */
