/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DELETEOBJECTHANDLER_H
#define DELETEOBJECTHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class DeleteObjectHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(DeleteObjectHandler);

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

#endif /* DELETEOBJECTHANDLER_H */
