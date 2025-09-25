/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "test/checker-fixture.hpp"
#include "base/scriptglobal.hpp"
#include "icinga/host.hpp"
#include "icinga/dependency.hpp"
#include "icinga/legacytimeperiod.hpp"
#include "test/utils.hpp"
#include <boost/test/unit_test.hpp>

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(checker, CheckerFixture, *boost::unit_test::label("checker"))

BOOST_AUTO_TEST_CASE(single_check)
{
	// For a single checkable, there shouldn't be any concurrent checks that trigger this event,
	// so we can safely use boost assertion macros within the event handler.
	Checkable::OnNextCheckChanged.connect([this](const Checkable::Ptr& checkable, const Value&) {
		BOOST_CHECK_EQUAL(checkable->GetName(), "host-1");
		BOOST_CHECK_LE(checkable->GetNextCheck(), checkable->GetLastCheck() + checkInterval + .5);
	});

	RegisterCheckablesRandom(1);
	SleepFor(200ms, true);

	BOOST_CHECK_EQUAL(2, testLogger->CountExpectedLogPattern("Executing check for 'host-1'"));
	BOOST_CHECK_EQUAL(2, testLogger->CountExpectedLogPattern("Check finished for object 'host-1'"));
	BOOST_CHECK_EQUAL(2, resultCount);
}

BOOST_AUTO_TEST_CASE(multi_checks)
{
	Checkable::OnNextCheckChanged.connect([this](const Checkable::Ptr& checkable, const Value&) {
		BOOST_CHECK_LE(checkable->GetNextCheck(), checkable->GetLastCheck() + checkInterval + .5);
	});

	RegisterCheckablesRandom(8);
	SleepFor(300ms, true);

	CHECK_LOG_MESSAGE("Check finished for object .*", 0s);
	CHECK_LOG_MESSAGE("Executing check for .*", 0s);

	auto executedC = testLogger->CountExpectedLogPattern("Executing check for .*");
	auto finishedC = testLogger->CountExpectedLogPattern("Check finished for object .*");
	BOOST_CHECK_EQUAL(executedC, finishedC);
	// With 8 checkables and a check interval of 0.1s, we expect that each checkable is checked at least
	// twice, but some of them possibly up to 3 times, depending on the timing and OS scheduling behavior.
	BOOST_CHECK_MESSAGE(22 <= resultCount && resultCount <= 25, "got=" << resultCount);
}

BOOST_AUTO_TEST_CASE(disabled_checks)
{
	RegisterCheckablesRandom(4, true);
	SleepFor(200ms, true);

	auto failedC = testLogger->CountExpectedLogPattern("Skipping check for host .*: active host checks are disabled");
	BOOST_CHECK_MESSAGE(6 <= failedC && failedC <= 9, "got=" << failedC);

	auto rescheduleC = testLogger->CountExpectedLogPattern("Checks for checkable .* are disabled. Rescheduling check.");
	BOOST_CHECK_MESSAGE(6 <= rescheduleC && rescheduleC <= 9, "got=" << rescheduleC);
	BOOST_CHECK_EQUAL(failedC, rescheduleC);

	CHECK_NO_LOG_MESSAGE("Check finished for object .*", 0s);
	CHECK_NO_LOG_MESSAGE("Executing check for .*", 0s);
	BOOST_CHECK_EQUAL(0, resultCount);
}

BOOST_AUTO_TEST_CASE(globally_disabled_checks)
{
	IcingaApplication::GetInstance()->SetEnableHostChecks(false); // Disable active host checks globally

	RegisterCheckablesRandom(4);
	SleepFor(200ms, true);

	auto failedC = testLogger->CountExpectedLogPattern("Skipping check for host .*: active host checks are disabled");
	BOOST_CHECK_MESSAGE(6 <= failedC && failedC <= 9, "got=" << failedC);

	auto rescheduleC = testLogger->CountExpectedLogPattern("Checks for checkable .* are disabled. Rescheduling check.");
	BOOST_CHECK_MESSAGE(6 <= rescheduleC && rescheduleC <= 9, "got=" << rescheduleC);
	BOOST_CHECK_EQUAL(failedC, rescheduleC);

	CHECK_NO_LOG_MESSAGE("Check finished for object .*", 0s);
	CHECK_NO_LOG_MESSAGE("Executing check for .*", 0s);
	BOOST_CHECK_EQUAL(0, resultCount);
}

BOOST_AUTO_TEST_CASE(unreachable_checkables)
{
	// Create a dependency that makes the host unreachable (i.e. no checks should be executed).
	// This must be done before activating the actual child checkable, otherwise the checker will
	// immediately schedule a check before the dependency is in place.
	RegisterCheckablesRandom(4, false, true);
	SleepFor(200ms, true);

	auto failedC = testLogger->CountExpectedLogPattern("Skipping check for object .*: Dependency failed.");
	BOOST_CHECK_MESSAGE(6 <= failedC && failedC <= 9, "got=" << failedC);

	auto rescheduleC = testLogger->CountExpectedLogPattern("Checks for checkable .* are disabled. Rescheduling check.");
	BOOST_CHECK_MESSAGE(6 <= rescheduleC && rescheduleC <= 9, "got=" << rescheduleC);
	BOOST_CHECK_EQUAL(failedC, rescheduleC);

	CHECK_NO_LOG_MESSAGE("Check finished for object .*", 0s);
	CHECK_NO_LOG_MESSAGE("Executing check for .*", 0s);
	BOOST_CHECK_EQUAL(0, resultCount);
}

BOOST_AUTO_TEST_CASE(child_rescheduled_on_parent_recovery)
{
	checkInterval = 80; // 1m20s

	auto child = RegisterCheckable("child", "random", "", "", false, true);
	child->SetMaxCheckAttempts(1);
	ReceiveCheckResults(child, 1, ServiceCritical, nullptr);
	BOOST_CHECK_EQUAL(1, resultCount);

	auto parents = child->GetParents();
	BOOST_REQUIRE_EQUAL(parents.size(), 1);

	CheckableScheduleInfo csi;
	Checkable::OnRescheduleCheck.connect([&csi](const Checkable::Ptr& checkable, double nextCheck) {
		csi.Object = checkable;
		csi.NextCheck = nextCheck;
	});

	Checkable::Ptr parent = *parents.begin();
	// When the parent checkable changes its state, then the child checkable should be rescheduled immediately
	// using the special signal `Checkable::OnRescheduleCheck` signal without changing its actual `next_check`
	// ts. In that case, the resulting check result should have a different `schedule_start` timestamp than
	// `child->GetLastCheck()`.
	ReceiveCheckResults(parent, 1, ServiceOK, nullptr);
	BOOST_CHECK_EQUAL(2, resultCount); // 1 cr for the child and 1 cr for parent.

	SleepFor(100ms, true);
	Checkable::OnRescheduleCheck.disconnect_all_slots();

	// Since the used nextcheck ts is randomly chosen one, we can't wait till that time passes.
	// However, since that timestamp must still be < now + 60, so we can assert it that it's within that range.
	BOOST_CHECK_EQUAL(csi.Object, child);
	BOOST_CHECK_NE(csi.NextCheck, 0.0);
	BOOST_CHECK_LE(csi.NextCheck, Utility::GetTime() + 60);

	// 2 crs from the above ones and maybe the Utility::Random()%60 caused another check
	// to be executed within the above 100ms sleep, but it shouldn't be more than 3.
	BOOST_CHECK_LE(resultCount, 3);

	auto cr = lastResult.load();
	BOOST_REQUIRE(cr);
	if (resultCount == 2) {
		BOOST_CHECK_EQUAL(cr, parent->GetLastCheckResult());
		BOOST_CHECK_NE(cr, child->GetLastCheckResult());
	} else {
		BOOST_CHECK_NE(cr, parent->GetLastCheckResult());
		BOOST_CHECK_EQUAL(cr, child->GetLastCheckResult());
		BOOST_CHECK_NE(cr->GetScheduleStart(), child->GetLastCheck());
	}
}

BOOST_AUTO_TEST_CASE(parent_rescheduled_on_child_state_change)
{
	checkInterval = 80; // 1m20s

	auto child = RegisterCheckable("child", "random", "", "", false, true);
	child->SetMaxCheckAttempts(1);

	auto parents = child->GetParents();
	BOOST_REQUIRE_EQUAL(parents.size(), 1);
	Checkable::Ptr parent = *parents.begin();
	parent->SetMaxCheckAttempts(1);
	parent->SetEnableActiveChecks(true);

	ReceiveCheckResults(parent, 1, ServiceCritical, nullptr);
	BOOST_CHECK_EQUAL(1, resultCount);

	CheckableScheduleInfo csi;
	Checkable::OnRescheduleCheck.connect([&csi](const Checkable::Ptr& checkable, double nextCheck) {
		csi.Object = checkable;
		csi.NextCheck = nextCheck;
	});

	ReceiveCheckResults(child, 1, ServiceWarning, nullptr);
	BOOST_CHECK_EQUAL(2, resultCount);

	SleepFor(100ms, true);
	Checkable::OnRescheduleCheck.disconnect_all_slots();

	BOOST_CHECK_EQUAL(parent, csi.Object);
	BOOST_CHECK_NE(csi.NextCheck, 0.0);
	BOOST_CHECK_LE(csi.NextCheck, Utility::GetTime()); // Must be < now.

	auto cr = lastResult.load();
	BOOST_REQUIRE(cr);
	BOOST_CHECK_NE(cr, parent->GetLastCheckResult());
	BOOST_CHECK_EQUAL(cr, child->GetLastCheckResult());
	BOOST_CHECK_EQUAL(2, resultCount);
}

BOOST_AUTO_TEST_CASE(never_in_check_period)
{
	TimePeriod::Ptr period = new TimePeriod;
	period->SetName("never");
	period->SetUpdate(new Function("LegacyTimePeriod", LegacyTimePeriod::ScriptFunc, {"tp", "begin", "end"}), true);
	period->Register();
	period->PreActivate();
	period->Activate();

	// Register some checkables that are only checked during the "never" time period, which is never.
	(void)RegisterCheckable("host-1", "random", "never");
	(void)RegisterCheckable("host-2", "random", "never");
	(void)RegisterCheckable("host-3", "sleep", "never");
	(void)RegisterCheckable("host-4", "sleep", "never");

	SleepFor(200ms, true);

	BOOST_CHECK_EQUAL(4, nextCheckTimes.size());
	for (auto& [host, nextCheck] : nextCheckTimes) {
		BOOST_TEST_MESSAGE("Host " << std::quoted(host) << " -> next_check: " << std::fixed << std::setprecision(0)
			<< nextCheck << " expected: " << Convert::ToDouble(period->GetValidEnd()));
		// The checker should ignore the regular check interval and instead set the next check time based on the tp.
		BOOST_CHECK_EQUAL(nextCheck, Convert::ToDouble(period->GetValidEnd()));
	}

	// We expect that no checks are executed, and instead the checker reschedules the checks for the
	// next valid end time of the "never" time period, which is always 24h from now. So, we should see
	// 4 log messages about skipping the checks due to the time period, and nothing else.
	BOOST_CHECK_EQUAL(4, testLogger->CountExpectedLogPattern("Skipping check for object .*, as not in check period 'never', until .*"));

	CHECK_NO_LOG_MESSAGE("Checks for checkable .* are disabled. Rescheduling check.", 0s);
	CHECK_NO_LOG_MESSAGE("Check finished for object .*", 0s);
	CHECK_NO_LOG_MESSAGE("Executing check for .*", 0s);
	BOOST_CHECK_EQUAL(0, resultCount);
}

BOOST_AUTO_TEST_CASE(in_check_period)
{
	TimePeriod::Ptr period = new TimePeriod;
	period->SetName("24x7");
	period->SetRanges(
		new Dictionary{
			{"monday", "00:00-24:00"},
			{"tuesday", "00:00-24:00"},
			{"wednesday", "00:00-24:00"},
			{"thursday", "00:00-24:00"},
			{"friday", "00:00-24:00"},
			{"saturday", "00:00-24:00"},
			{"sunday", "00:00-24:00"}
		},
		true
	);
	period->SetUpdate(new Function("LegacyTimePeriod", LegacyTimePeriod::ScriptFunc, {"tp", "begin", "end"}), true);
	period->Register();
	period->PreActivate();
	period->Activate();

	// Register some checkables that are only checked during the "24x7" time period, which is always.
	(void)RegisterCheckable("host-1", "random", "24x7");
	(void)RegisterCheckable("host-2", "random", "24x7");
	(void)RegisterCheckable("host-3", "sleep", "24x7");
	(void)RegisterCheckable("host-4", "sleep", "24x7");

	SleepFor(300ms, true);

	// We expect that checks are executed normally, and the checker sets the next check time based
	// on the regular check interval. So, we should see multiple checks executed for each checkable.
	CHECK_LOG_MESSAGE("Executing check for .*", 0s);
	CHECK_LOG_MESSAGE("Check finished for object .*", 0s);

	CHECK_NO_LOG_MESSAGE("Skipping check for object .*: Dependency failed.", 0s);
	CHECK_NO_LOG_MESSAGE("Check for checkable checkable .* are disabled. Rescheduling check.", 0s);

	BOOST_CHECK_MESSAGE(5 <= resultCount && resultCount <= 8, "got=" << resultCount);
}

BOOST_AUTO_TEST_CASE(max_concurrent_checks)
{
	// Limit the number of concurrent checks to 4.
	ScriptGlobal::Set("MaxConcurrentChecks", 4);

	// Register 16 checkables that each sleep for 10 seconds when executing their check.
	// With a max concurrent check limit of 4, we should see that only 4 checks are executed
	// at the same time, and the remaining 12 checks are queued until one of the running checks
	// finishes (which will not happen within the short sleep time of this test).
	RegisterCheckablesSleep(16, 10);
	std::this_thread::sleep_for(300ms);

	auto objects(ConfigType::GetObjectsByType<Host>());
	BOOST_CHECK_EQUAL(16, objects.size());

	for (auto& h : objects) {
		// Force a reschedule of the checks to see whether the checker does absolutely nothing
		// when the max concurrent check limit is reached. Normally, this would force the checker
		// to immediately pick up the checkable and execute its check, but since all 4 slots are
		// already taken, the checker should just update its queue idx and do nothing else.
		Checkable::OnRescheduleCheck(h, Utility::GetTime());
	}
	std::this_thread::sleep_for(300ms);

	// We expect that only 4 checks are started initially, and the other 12 checks should have
	// never been run, since the sleep time for each check (10 seconds) is much longer than the
	// total sleep time of this test (600ms).
	CHECK_LOG_MESSAGE("Pending checkables: 4; Idle checkables: 12; Checks/s: .*", 0s);
	BOOST_CHECK_EQUAL(4, testLogger->CountExpectedLogPattern("Scheduling info for checkable .*: Object .*"));
	BOOST_CHECK_EQUAL(4, testLogger->CountExpectedLogPattern("Executing check for .*"));
	BOOST_CHECK_EQUAL(4, Checkable::GetPendingChecks());

	CHECK_NO_LOG_MESSAGE("Check finished for object .*", 0s); // none finished yet
	BOOST_CHECK_EQUAL(0, resultCount);
}

BOOST_AUTO_TEST_CASE(skipped_remote_checks)
{
	// The check execution for remote checks is skipped if the remote endpoint is not connected,
	// not syncing, and we are within the cold startup window (5min after application start).
	Application::GetInstance()->SetStartTime(Utility::GetTime());

	// Set the check and retry intervals to 60 seconds, since it's sufficient to
	// just verify that the checks are skipped only once, and not repeatedly.
	checkInterval = 60;
	retryInterval = 60;

	RegisterRemoteChecks(8);
	SleepFor(200ms, true);

	BOOST_CHECK_EQUAL(8, nextCheckTimes.size());
	for (auto& [host, nextCheck] : nextCheckTimes) {
		BOOST_TEST_MESSAGE("Host " << std::quoted(host) << " -> next_check: " << std::fixed << std::setprecision(0)
			<< nextCheck << " roughly expected: " << Utility::GetTime() + checkInterval);
		// Our algorithm for computing the next check time is not too precise, but it should roughly be within 5s of
		// the expected next check time based on the check interval. See Checkable::UpdateNextCheck() for details.
		BOOST_CHECK_GE(nextCheck, Utility::GetTime() + checkInterval-5);
		BOOST_CHECK_LE(nextCheck, Utility::GetTime() + checkInterval); // but not more than the interval
	}

	BOOST_CHECK_EQUAL(nullptr, lastResult.load()); // No check results should be received
	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Executing check for .*"));
	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Check finished for object .*"));

	CHECK_NO_LOG_MESSAGE("Skipping check for object .*: Dependency failed.", 0s);
	CHECK_NO_LOG_MESSAGE("Checks for checkable .* are disabled. Rescheduling check.", 0s);

	BOOST_CHECK_EQUAL(0, resultCount);
}

BOOST_AUTO_TEST_CASE(remote_checks_outside_cold_startup)
{
	Application::GetInstance()->SetStartTime(Utility::GetTime()-500); // Simulate being outside cold startup window

	checkInterval = 60;
	retryInterval = 60;

	RegisterRemoteChecks(8);
	SleepFor(200ms, true);

	BOOST_CHECK_EQUAL(8, nextCheckTimes.size());
	for (auto& [host, nextCheck] : nextCheckTimes) {
		BOOST_TEST_MESSAGE("Host " << std::quoted(host) << " -> next_check: " << std::fixed << std::setprecision(0)
			<< nextCheck << " roughly expected: " << Utility::GetTime() + checkInterval);
		// Our algorithm for computing the next check time is not too precise, but it should roughly be within 5s of
		// the expected next check time based on the check interval. See Checkable::UpdateNextCheck() for details.
		BOOST_CHECK_GE(nextCheck, Utility::GetTime() + checkInterval-5);
		BOOST_CHECK_LE(nextCheck, Utility::GetTime() + checkInterval); // but not more than the interval
	}

	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Executing check for .*"));
	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Check finished for object .*"));
	BOOST_CHECK_EQUAL(8, resultCount);

	CHECK_NO_LOG_MESSAGE("Skipping check for object .*: Dependency failed.", 0s);
	CHECK_NO_LOG_MESSAGE("Checks for checkable .* are disabled. Rescheduling check.", 0s);

	BOOST_REQUIRE(lastResult.load()); // We should now have a cr!

	auto cr = lastResult.load();
	BOOST_CHECK_EQUAL(ServiceUnknown, cr->GetState());
	String expectedOutput = "Remote Icinga instance 'remote-checker' is not connected to '" + Endpoint::GetLocalEndpoint()->GetName() + "'";
	BOOST_CHECK_EQUAL(expectedOutput, cr->GetOutput());
}

BOOST_AUTO_TEST_CASE(remote_checks_with_connected_endpoint)
{
	// Register a remote endpoint that is connected, so remote checks can be executed or actually simulate
	// sending the check command to the remote endpoint. In this case, we shouldn't also receive any cr,
	// but checker should reschedule the check at "now + check_timeout (which is 0) + 30s".
	RegisterRemoteChecks(8, true);
	SleepFor(200ms, true);

	BOOST_CHECK_EQUAL(8, nextCheckTimes.size());
	for (auto& [host, nextCheck] : nextCheckTimes) {
		// In this case, there shouldn't be any OnNextCheckChanged events triggered,
		// thus nextCheck should still be 0.0 as initialized by RegisterCheckable().
		BOOST_CHECK_EQUAL(0.0, nextCheck);
	}

	auto hosts(ConfigType::GetObjectsByType<Host>());
	BOOST_CHECK_EQUAL(8, hosts.size());

	for (auto& h : hosts) {
		// Verify the next_check time is set to roughly now + 30s, since RegisterCheckable()
		// initializes the check_timeout to 0.
		BOOST_TEST_MESSAGE("Host " << std::quoted(h->GetName().GetData()) << " -> next_check: " << std::fixed
			<< std::setprecision(0) << h->GetNextCheck() << " roughly expected: " << Utility::GetTime() + 30);
		BOOST_CHECK_GE(h->GetNextCheck(), Utility::GetTime() + 25);
		BOOST_CHECK_LE(h->GetNextCheck(), Utility::GetTime() + 30);
	}

	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Sending message 'event::ExecuteCommand' to 'remote-checker'"));
	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Executing check for .*"));
	BOOST_CHECK_EQUAL(8, testLogger->CountExpectedLogPattern("Check finished for object .*"));

	CHECK_NO_LOG_MESSAGE("Skipping check for object .*: Dependency failed.", 0s);
	CHECK_NO_LOG_MESSAGE("Checks for checkable .* are disabled. Rescheduling check.", 0s);

	BOOST_CHECK_EQUAL(0, resultCount); // No check results should be received yet
}

BOOST_AUTO_TEST_SUITE_END()
