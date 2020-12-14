/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"
#include "remote/endpoint.hpp"

namespace icinga
{

/**
 * A Command database object.
 *
 * @ingroup ido
 */
class EndpointDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(EndpointDbObject);

	EndpointDbObject(const intrusive_ptr<DbType>& type, const String& name1, const String& name2);

	static void StaticInitialize();

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;

private:
	static void UpdateConnectedStatus(const Endpoint::Ptr& endpoint);
	static int EndpointIsConnected(const Endpoint::Ptr& endpoint);
};

}
