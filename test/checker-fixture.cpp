/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "config/configcompiler.hpp"
#include "remote/apilistener.hpp"
#include "test/checker-fixture.hpp"
#include "test/icingaapplication-fixture.hpp"

using namespace icinga;

CheckerFixture::CheckerFixture()
{
	checker = new CheckerComponent;
	checker->SetResultTimerInterval(.4); // Speed up result timer for tests
	checker->SetName("randomizer", true);
	checker->Register();
	checker->OnConfigLoaded();
	checker->PreActivate();
	checker->Activate();

	// Manually building and registering the command won't work here as we need a real callable
	// function that produces cr and calls ProcessCheckResult on the checkable. So we use a small
	// config snippet that imports the random and sleep check commands registered by the "methods-itl.cpp"
	// file in the methods lib.
	ConfigItem::RunWithActivationContext(
		new Function{
			"checker-fixture",
			[] {
				std::unique_ptr<Expression> expression = ConfigCompiler::CompileText(
					"<checker-fixture>",
					R"CONFIG(
object CheckCommand "random" {
	import "random-check-command"
}

object CheckCommand "sleep" {
    import "sleep-check-command"
}
)CONFIG"
				);
				BOOST_REQUIRE(expression);
				ScriptFrame frame(true);
				BOOST_CHECK_NO_THROW(expression->Evaluate(frame));
			}
		}
	);

	Checkable::OnNewCheckResult.connect(
		[this](const Checkable::Ptr&, const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
			++resultCount;
			lastResult.store(cr);
		}
	);
	// Track the next check times of the checkables, so we can verify that they are set to the expected
	// value. The map is populated with the expected checkable names by the RegisterChecable() function.
	// So, we can safely modify the map from within this signal handler without further locking, since there
	// won't be any concurrent access to the same keys.
	Checkable::OnNextCheckChanged.connect([this](const Checkable::Ptr& checkable, const Value&) {
		assert(nextCheckTimes.find(checkable->GetName()) != nextCheckTimes.end());
		nextCheckTimes[checkable->GetName()] = checkable->GetNextCheck();
	});
}

CheckerFixture::~CheckerFixture()
{
	checker->Deactivate();
}

void CheckerFixture::RegisterCheckablesRandom(int count, bool disableChecks, bool unreachable)
{
	for (int i = 1; i <= count; ++i) {
		RegisterCheckable("host-" + std::to_string(i), "random", "", "", disableChecks, unreachable);
	}
}

void CheckerFixture::RegisterCheckablesSleep(int count, double sleepTime, bool disableChecks, bool unreachable)
{
	for (int i = 1; i <= count; ++i) {
		auto h = RegisterCheckable("host-" + std::to_string(i), "sleep", "", "", disableChecks, unreachable);
		h->SetVars(new Dictionary{{"sleep_time", sleepTime}});
	}
}

void CheckerFixture::RegisterRemoteChecks(int count, bool isConnected, bool isSyncingReplayLogs)
{
	RegisterEndpoint("remote-checker", isConnected, isSyncingReplayLogs);
	for (int i = 1; i <= count; ++i) {
		auto h = RegisterCheckable("host-"+std::to_string(i), "random", "", "remote-checker");
		Checkable::OnRescheduleCheck(h, Utility::GetTime()); // Force initial scheduling
	}
}

Host::Ptr CheckerFixture::RegisterCheckable(
	std::string name,
	std::string cmd,
	std::string period,
	std::string endpoint,
	bool disableChecks,
	bool unreachable
)
{
	Host::Ptr host = new Host;
	host->SetName(std::move(name), true);
	host->SetCheckCommandRaw(std::move(cmd), true);
	host->SetCheckInterval(checkInterval, true);
	host->SetRetryInterval(retryInterval, true);
	host->SetHAMode(HARunEverywhere, true); // Disable HA for tests
	host->SetEnableActiveChecks(!disableChecks, true);
	host->SetCheckPeriodRaw(std::move(period), true);
	host->SetZoneName(endpoint, true);
	host->SetCommandEndpointRaw(std::move(endpoint), true);
	host->SetCheckTimeout(0, true);
	host->Register();
	host->OnAllConfigLoaded();

	nextCheckTimes[host->GetName()] = 0.0; // Initialize next check time tracking

	if (unreachable) {
		Host::Ptr parent = new Host;
		parent->SetName(Utility::NewUniqueID(), true);
		parent->SetStateRaw(ServiceCritical, true);
		parent->SetStateType(StateTypeHard, true);
		parent->SetLastCheckResult(new CheckResult, true);
		parent->Register();

		Dependency::Ptr dep = new Dependency;
		dep->SetName(Utility::NewUniqueID(), true);
		dep->SetStateFilter(StateFilterUp, true);
		dep->SetDisableChecks(true, true);
		dep->SetParent(parent);
		dep->SetChild(host);
		dep->Register();

		host->AddDependency(dep);
	}

	host->PreActivate();
	host->Activate();
	return host;
}

void CheckerFixture::SleepFor(double seconds, bool deactivateLogger) const
{
	Utility::Sleep(seconds);
	Checkable::OnNextCheckChanged.disconnect_all_slots();
	Checkable::OnNewCheckResult.disconnect_all_slots();
	if (deactivateLogger) {
		DeactivateLogger();
	}
}

Endpoint::Ptr CheckerFixture::RegisterEndpoint(std::string name, bool isConnected, bool isSyncingReplayLogs)
{
	auto makeEndpoint = [](const std::string& name, bool syncing) {
		Endpoint::Ptr remote = new Endpoint;
		remote->SetName(name, true);
		if (syncing) {
			remote->SetSyncing(true);
		}
		remote->Register();
		remote->PreActivate();
		return remote;
	};

	Endpoint::Ptr remote = makeEndpoint(name, isSyncingReplayLogs);
	Endpoint::Ptr local = makeEndpoint("local-tester", false);

	Zone::Ptr zone = new Zone;
	zone->SetName(remote->GetName(), true);
	zone->SetEndpointsRaw(new Array{{remote->GetName(), local->GetName()}}, true);
	zone->Register();
	zone->OnAllConfigLoaded();
	zone->PreActivate();
	zone->Activate();

	ApiListener::Ptr listener = new ApiListener;
	listener->SetIdentity(local->GetName(), true);
	listener->SetName(local->GetName(), true);
	listener->Register();
	try {
		listener->OnAllConfigLoaded(); // Initialize the m_LocalEndpoint of the listener!

		// May throw due to various reasons, but we only care that the m_Instance singleton
		// is set which it will be if no other ApiListener is registered yet.
		listener->OnConfigLoaded();
	} catch (const std::exception& ex) {
		BOOST_TEST_MESSAGE("Exception during ApiListener::OnConfigLoaded: " << DiagnosticInformation(ex));
	}

	if (isConnected) {
		JsonRpcConnection::Ptr client = new JsonRpcConnection(
			new StoppableWaitGroup,
			"anonymous",
			false,
			nullptr,
			RoleClient
		);
		remote->AddClient(client);
	}
	return remote;
}
