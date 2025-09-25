/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/atomic.hpp"
#include "checker/checkercomponent.hpp"
#include "icinga/checkcommand.hpp"
#include "remote/endpoint.hpp"
#include "test/base-testloggerfixture.hpp"

namespace icinga {

/**
 * Test fixture for tests involving the @c CheckerComponent.
 *
 * This fixture sets up a CheckerComponent instance and provides utility functions to register
 * checkable objects. It is derived from @c TestLoggerFixture to capture log output during tests,
 * so tests can verify that expected log messages are produced. The fixture also connects to the
 * @c Checkable::OnNewCheckResult signal to count the number of check results produced during tests.
 */
struct CheckerFixture : TestLoggerFixture
{
	CheckerFixture();

	~CheckerFixture();

	/**
	 * Registers a fully configured set of checkable hosts that execute the "random" check command.
	 *
	 * Each host is configured with a random check command, check interval, and retry interval.
	 * If @c unreachable is true, each host is made unreachable by adding a dependency on a parent
	 * host that is in a critical state. This prevents the checker from executing checks for the
	 * child hosts. The check and retry intervals are kept low to allow for quick test execution,
	 * but they can be adjusted via the @c interval and @c retry parameters.
	 *
	 * @param count Number of checkable hosts to register.
	 * @param disableChecks If true, disables active checks for each host.
	 * @param unreachable If true, makes each host unreachable via a dependency.
	 */
	void RegisterCheckablesRandom(int count, bool disableChecks = false, bool unreachable = false);

	/**
	 * Registers a fully configured set of checkable hosts that execute the "sleep" command.
	 *
	 * Each host is configured with a sleep check command that sleeps for the specified duration.
	 * The check and retry intervals can be adjusted via the @c checkInterval and @c retryInterval
	 * member variables of the fixture. If @c unreachable is true, each host is made unreachable by
	 * adding a dependency on a parent host that is in a critical state. This prevents the checker
	 * from executing checks for the child hosts.
	 *
	 * @param count Number of checkable hosts to register.
	 * @param sleepTime Duration (in seconds) that the sleep command should sleep. Defaults to 1.0 second.
	 * @param disableChecks If true, disables active checks for each host.
	 * @param unreachable If true, makes each host unreachable via a dependency.
	 */
	void RegisterCheckablesSleep(
		int count,
		double sleepTime = 1.0,
		bool disableChecks = false,
		bool unreachable = false
	);

	/**
	 * Registers a remote endpoint and a set of checkable hosts assigned to that endpoint.
	 *
	 * The remote endpoint can be configured to appear connected or disconnected, and can also
	 * be set to be syncing replay as needed for tests involving remote checks.
	 *
	 * @param count Number of checkable hosts to register.
	 * @param isConnected If true, the remote endpoint is marked as connected.
	 * @param isSyncingReplayLogs If true, the remote endpoint is marked as syncing replay logs.
	 */
	void RegisterRemoteChecks(int count, bool isConnected = false, bool isSyncingReplayLogs = false);

	Host::Ptr RegisterCheckable(
		std::string name,
		std::string cmd,
		std::string period = "",
		std::string endpoint = "",
		bool disableChecks = false,
		bool unreachable = false
	);

	/**
	 * Sleeps for the specified number of seconds, then immediately disconnects all signal handlers
	 * connected to @c Checkable::OnNextCheckChanged and @c Checkable::OnNewCheckResult.
	 *
	 * This is useful in tests to allow some time for checks to be executed and results to be processed,
	 * while ensuring that no further signal handlers are called after the sleep period. This helps to avoid
	 * unexpected side effects in tests, since the checker continues to run till the fixture is destroyed.
	 *
	 * @param seconds Number of seconds to sleep.
	 * @param deactivateLogger If true, deactivates the test logger after sleeping to prevent further log capture.
	 */
	void SleepFor(double seconds, bool deactivateLogger = false) const;

	/**
	 * Registers a remote endpoint with the specified name and connection/syncing state and returns it.
	 *
	 * @param name Name of the endpoint to register.
	 * @param isConnected If true, the endpoint is marked as connected.
	 * @param isSyncingReplayLogs If true, the endpoint is marked as syncing replay logs.
	 *
	 * @return The registered endpoint instance.
	 */
	static Endpoint::Ptr RegisterEndpoint(std::string name, bool isConnected = false, bool isSyncingReplayLogs = false);

	/**
	 * resultCount tracks the number of check results produced by the checker.
	 *
	 * This is used in tests to verify that checks are actually being executed and results processed.
	 * It is incremented from within the OnNewCheckResult signal handler, thus must be atomic.
	 */
	Atomic<int> resultCount{0};
	AtomicOrLocked<CheckResult::Ptr> lastResult; // Stores the last check result received, for inspection in tests.
	/**
	 * nextCheckTimes tracks the next scheduled check time for each registered checkable host.
	 * This might not be used in all tests, but is available for tests that need to verify the exact
	 * next check timestamp set by the checker.
	 */
	std::map<std::string /* host */, double /* next_check */> nextCheckTimes;
	double checkInterval{.1}; // Interval in seconds between regular checks for each checkable.
	double retryInterval{.1}; // Interval in seconds between retry checks for each checkable.
	CheckerComponent::Ptr checker;
};

} // namespace icinga
