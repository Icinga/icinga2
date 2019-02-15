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
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params
	) override;
};

}

#endif /* MODIFYOBJECTHANDLER_H */
