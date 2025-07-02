/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ACTIONSHANDLER_H
#define ACTIONSHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ActionsHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ActionsHandler);

	static thread_local ApiUser::Ptr AuthenticatedApiUser;

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

#endif /* ACTIONSHANDLER_H */
