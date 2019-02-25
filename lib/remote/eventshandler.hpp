/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EVENTSHANDLER_H
#define EVENTSHANDLER_H

#include "remote/httphandler.hpp"
#include "remote/eventqueue.hpp"

namespace icinga
{

class EventsHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(EventsHandler);

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;
};

}

#endif /* EVENTSHANDLER_H */
