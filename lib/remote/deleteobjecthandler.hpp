/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DELETEOBJECTHANDLER_H
#define DELETEOBJECTHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class DeleteObjectHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(DeleteObjectHandler);

	bool HandleRequest(
		AsioTlsStream& stream,
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params,
		boost::asio::yield_context& yc,
		bool& hasStartedStreaming
	) override;
};

}

#endif /* DELETEOBJECTHANDLER_H */
