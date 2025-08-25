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
		const HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc
	) override;

private:
	void HandleGet(const HttpRequest& request, HttpResponse& response);
	void HandlePost(const HttpRequest& request, HttpResponse& response);
	void HandleDelete(const HttpRequest& request, HttpResponse& response);

};

}

#endif /* CONFIGMODULESHANDLER_H */
