/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef INFOHANDLER_H
#define INFOHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class InfoHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(InfoHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}

#endif /* INFOHANDLER_H */
