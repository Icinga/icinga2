/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ACTIONSHANDLER_H
#define ACTIONSHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ActionsHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ActionsHandler);

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;
};

}

#endif /* ACTIONSHANDLER_H */
