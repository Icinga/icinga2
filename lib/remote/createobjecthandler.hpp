/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CREATEOBJECTHANDLER_H
#define CREATEOBJECTHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class CreateObjectHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(CreateObjectHandler);

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

#endif /* CREATEOBJECTHANDLER_H */
