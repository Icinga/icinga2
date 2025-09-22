/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TYPEQUERYHANDLER_H
#define TYPEQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class TypeQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(TypeQueryHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}

#endif /* TYPEQUERYHANDLER_H */
