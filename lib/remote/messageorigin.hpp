/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/zone.hpp"
#include "remote/jsonrpcconnection.hpp"

namespace icinga
{

/**
 * @ingroup remote
 */
class MessageOrigin final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(MessageOrigin);

	Zone::Ptr FromZone;
	JsonRpcConnection::Ptr FromClient;

	bool IsLocal() const;
};

}
