/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STATUSHANDLER_H
#define STATUSHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class StatusHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(StatusHandler);

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;
};

}

#endif /* STATUSHANDLER_H */
