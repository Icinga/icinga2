/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGSTAGESHANDLER_H
#define CONFIGSTAGESHANDLER_H

#include "base/atomic.hpp"
#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigStagesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigStagesHandler);

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

#endif /* CONFIGSTAGESHANDLER_H */
