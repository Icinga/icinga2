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

	bool HandleRequest(
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params
	) override;
};

}

#endif /* EVENTSHANDLER_H */
