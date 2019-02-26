/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef INFOHANDLER_H
#define INFOHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class InfoHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(InfoHandler);

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;
};

}

#endif /* INFOHANDLER_H */
