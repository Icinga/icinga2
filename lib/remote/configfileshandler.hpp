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

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;
};

}

#endif /* CONFIGFILESHANDLER_H */
