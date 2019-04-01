/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OBJECTQUERYHANDLER_H
#define OBJECTQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ObjectQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ObjectQueryHandler);

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

private:
	static Dictionary::Ptr SerializeObjectAttrs(const Object::Ptr& object, const String& attrPrefix,
		const Array::Ptr& attrs, bool isJoin, bool allAttrs);
};

}

#endif /* OBJECTQUERYHANDLER_H */
