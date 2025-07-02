/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGMODULESHANDLER_H
#define CONFIGMODULESHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigPackagesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigPackagesHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		AsioTlsStream& stream,
		HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc,
		HttpServerConnection& server
	) override;

private:
	void HandleGet(HttpRequest& request, HttpResponse& response);
	void HandlePost(HttpRequest& request, HttpResponse& response);
	void HandleDelete(HttpRequest& request, HttpResponse& response);

};

}

#endif /* CONFIGMODULESHANDLER_H */
