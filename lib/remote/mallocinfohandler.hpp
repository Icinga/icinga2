/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/httphandler.hpp"

namespace icinga
{

class MallocInfoHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(MallocInfoHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}
