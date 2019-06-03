/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APIUSER_H
#define APIUSER_H

#include "remote/i2-remote.hpp"
#include "remote/apiuser-ti.hpp"

namespace icinga
{

/**
 * @ingroup remote
 */
class ApiUser final : public ObjectImpl<ApiUser>
{
public:
	DECLARE_OBJECT(ApiUser);
	DECLARE_OBJECTNAME(ApiUser);

	static ApiUser::Ptr GetByClientCN(const String& cn);
	static ApiUser::Ptr GetByAuthHeader(const String& auth_header);
};

}

#endif /* APIUSER_H */
