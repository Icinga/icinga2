/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EVENTSHANDLER_H
#define EVENTSHANDLER_H

#include "remote/httphandler.hpp"
#include "remote/eventqueue.hpp"

namespace icinga
{

class EventsHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(EventsHandler);

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

#endif /* EVENTSHANDLER_H */
