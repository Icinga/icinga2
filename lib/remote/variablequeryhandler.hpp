/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef VARIABLEQUERYHANDLER_H
#define VARIABLEQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class VariableQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(VariableQueryHandler);

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

#endif /* VARIABLEQUERYHANDLER_H */
