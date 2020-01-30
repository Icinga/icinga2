/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
