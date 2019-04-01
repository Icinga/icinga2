/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STATUSHANDLER_H
#define STATUSHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class StatusHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(StatusHandler);

	bool HandleRequest(
		AsioTlsStream& stream,
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params,
		boost::asio::yield_context& yc,
		bool& hasStartedStreaming
	) override;
};

}

#endif /* STATUSHANDLER_H */
