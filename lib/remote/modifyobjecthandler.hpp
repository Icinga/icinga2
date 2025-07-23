/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef MODIFYOBJECTHANDLER_H
#define MODIFYOBJECTHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ModifyObjectHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ModifyObjectHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}

#endif /* MODIFYOBJECTHANDLER_H */
