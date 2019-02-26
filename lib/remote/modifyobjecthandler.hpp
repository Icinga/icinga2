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

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;
};

}

#endif /* MODIFYOBJECTHANDLER_H */
