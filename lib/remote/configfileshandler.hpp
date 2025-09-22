/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGFILESHANDLER_H
#define CONFIGFILESHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigFilesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigFilesHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}

#endif /* CONFIGFILESHANDLER_H */
