/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ZONE_H
#define ZONE_H

#include "remote/i2-remote.hpp"
#include "remote/zone-ti.hpp"
#include "remote/endpoint.hpp"

namespace icinga
{

/**
 * @ingroup remote
 */
class Zone final : public ObjectImpl<Zone>
{
public:
	DECLARE_OBJECT(Zone);
	DECLARE_OBJECTNAME(Zone);

	void OnAllConfigLoaded() override;

	Zone::Ptr GetParent() const;
	std::set<Endpoint::Ptr> GetEndpoints() const;
	std::vector<Zone::Ptr> GetAllParentsRaw() const;
	Array::Ptr GetAllParents() const override;

	bool CanAccessObject(const ConfigObject::Ptr& object);
	bool IsChildOf(const Zone::Ptr& zone);
	bool IsGlobal() const;
	bool IsHACluster() const;

	static Zone::Ptr GetLocalZone();

protected:
	void ValidateEndpointsRaw(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

private:
	Zone::Ptr m_Parent;
	std::vector<Zone::Ptr> m_AllParents;
};

}

#endif /* ZONE_H */
