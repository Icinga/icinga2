/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef MESSAGEORIGIN_H
#define MESSAGEORIGIN_H

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

#endif /* MESSAGEORIGIN_H */
