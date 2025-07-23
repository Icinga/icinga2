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
		const WaitGroup::Ptr& waitGroup,
		AsioTlsStream& stream,
		const HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc,
		HttpServerConnection& server
	) override;
};

}

#endif /* STATUSHANDLER_H */
