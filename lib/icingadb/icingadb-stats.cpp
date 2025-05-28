/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadb.hpp"
#include "base/application.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/statsfunction.hpp"
#include "base/convert.hpp"

using namespace icinga;

Dictionary::Ptr IcingaDB::GetStats()
{
	Dictionary::Ptr status = new Dictionary();
	IcingaApplication::StatsFunc(status, nullptr);

	Dictionary::Ptr app(Dictionary::Ptr(status->Get("icingaapplication"))->Get("app"));
	app->Set("program_start", TimestampToMilliseconds(Application::GetStartTime()));

	if (auto localEndpoint(Endpoint::GetLocalEndpoint()); localEndpoint) {
		app->Set("endpoint_id", GetObjectIdentifier(localEndpoint));
	}

	return new Dictionary{{ "IcingaApplication", new Dictionary{{"status", status}}}};
}
