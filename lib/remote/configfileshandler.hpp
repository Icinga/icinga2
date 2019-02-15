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
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params
	) override;
};

}

#endif /* CONFIGFILESHANDLER_H */
