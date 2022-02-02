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
	Dictionary::Ptr stats = new Dictionary();

	//TODO: Figure out if more stats can be useful here.
	Namespace::Ptr statsFunctions = ScriptGlobal::Get("StatsFunctions", &Empty);

	if (!statsFunctions)
		Dictionary::Ptr();

	statsFunctions = statsFunctions->ShallowClone();
	ObjectLock olock(statsFunctions);

	for (auto& kv : statsFunctions)
	{
		Function::Ptr func = kv.second->Get();

		if (!func)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid status function name."));

		Dictionary::Ptr status = new Dictionary();
		Array::Ptr perfdata = new Array();
		func->Invoke({ status, perfdata });

		stats->Set(kv.first, new Dictionary({
			{ "status", status },
			{ "perfdata", Serialize(perfdata, FAState) }
		}));
	}

	typedef Dictionary::Ptr DP;
	DP app = DP(DP(DP(stats->Get("IcingaApplication"))->Get("status"))->Get("icingaapplication"))->Get("app");

	app->Set("program_start", TimestampToMilliseconds(Application::GetStartTime()));

	auto localEndpoint (Endpoint::GetLocalEndpoint());
	if (localEndpoint) {
		app->Set("endpoint_id", GetObjectIdentifier(localEndpoint));
	}

	return stats;
}

