/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGSTAGESHANDLER_H
#define CONFIGSTAGESHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigStagesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigStagesHandler);

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

#endif /* CONFIGSTAGESHANDLER_H */
