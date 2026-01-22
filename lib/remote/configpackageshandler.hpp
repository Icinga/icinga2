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
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;

private:
	void HandleGet(const HttpApiRequest& request, HttpApiResponse& response);
	void HandlePost(const HttpApiRequest& request, HttpApiResponse& response);
	void HandleDelete(const HttpApiRequest& request, HttpApiResponse& response);

};

}

#endif /* CONFIGMODULESHANDLER_H */
