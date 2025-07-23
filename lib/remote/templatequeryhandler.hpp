/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TEMPLATEQUERYHANDLER_H
#define TEMPLATEQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class TemplateQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(TemplateQueryHandler);

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

#endif /* TEMPLATEQUERYHANDLER_H */
