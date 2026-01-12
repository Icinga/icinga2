/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "base/defer.hpp"
#include "remote/apilistener.hpp"
#include "test/base-testloggerfixture.hpp"
#include "config/configcompiler.hpp"
#include "notification/notificationcomponent.hpp"

using namespace icinga;

namespace {

/**
 * Gets the pointer to the private NotificationTimerHandler() by using Friend-Injection.
 *
 * This uses the exception the standard makes to private member access for explicit template
 * instantiation by instantiating a type that defines an accessor to the member function pointer
 * in the surrounding anonymous namespace.
 *
 * The reason for the anonymous namespace is that it doesn't violate the ODR if other
 * instantiations are made to this template in other translation units.
 * This isn't actually an issue here because the name is very specific to the single use-case of
 * obtaining access to NotificationTimerHandler().
 */
template<auto privateMemberFnPtr>
struct InvokeTimerHandlerImpl
{
	friend void InvokeTimerHandler(const NotificationComponent::Ptr& nc) { (*nc.*privateMemberFnPtr)(); }
};
void InvokeTimerHandler(const NotificationComponent::Ptr& nc);

template struct InvokeTimerHandlerImpl<&NotificationComponent::NotificationTimerHandler>;

} // namespace

class NotificationComponentFixture : public TestLoggerFixture
{
public:
	NotificationComponentFixture()
	{
		auto createObjects = []() {
			String config = R"CONFIG({
object CheckCommand "dummy" {
	command = "/bin/echo"
}
object Host "h1" {
	address = "h1"
	check_command = "dummy"
	enable_notifications = true
	enable_active_checks = false
	enable_passive_checks = true
}
object NotificationCommand "send" {
	command = ["true"]
}
apply Notification "n1" to Host {
	interval = 0
	command = "send"
	period = "tp1"
	users = [ "u1" ]
	assign where host.enable_notifications == true
}
object User "u1" {
	enable_notifications = true
}
object TimePeriod "tp1" {
	display_name = "Test TimePeriod"
	ranges = {
		"monday" = "00:00-24:00"
		"tuesday" = "00:00-24:00"
		"wednesday" = "00:00-24:00"
		"thursday" = "00:00-24:00"
		"friday" = "00:00-24:00"
		"saturday" = "00:00-24:00"
		"sunday" = "00:00-24:00"
  }
}
object NotificationComponent "nc" {}
})CONFIG";
			std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
			expr->Evaluate(*ScriptFrame::GetCurrentFrame());
		};

		auto ret = ConfigItem::RunWithActivationContext(new Function("CreateTestObjects", createObjects));
		BOOST_REQUIRE(ret);

		m_Host = Host::GetByName("h1");
		BOOST_REQUIRE(m_Host);

		m_Notification = Notification::GetByName("h1!n1");
		BOOST_REQUIRE(m_Notification);

		m_TimePeriod = TimePeriod::GetByName("tp1");
		BOOST_REQUIRE(m_TimePeriod);

		ApiListener::UpdateObjectAuthority();
		BOOST_REQUIRE(ApiListener::UpdatedObjectAuthority());

		// Store the old periods from the config snippets to reuse them later.
		m_AllTimePeriod = m_TimePeriod->GetRanges();

		Checkable::OnNotificationSentToUser.connect(NotificationSentToUserHandler);
	}

	static void NotificationSentToUserHandler(
		const Notification::Ptr&,
		const Checkable::Ptr&,
		const User::Ptr&,
		const NotificationType& type,
		const CheckResult::Ptr&,
		const String&,
		const String&,
		const String&,
		const MessageOrigin::Ptr&
	)
	{
		std::unique_lock lock(m_NotificationMutex);
		m_LastNotification = type;
		m_NumNotifications++;
		lock.unlock();
		m_NotificationCv.notify_all();
	}

	auto WaitForExpectedNotificationCount(std::size_t expectedCount, std::chrono::milliseconds timeout = 5s)
	{
		Defer clearLog{[this]() { ClearTestLogger(); }};
		std::unique_lock lock(m_NotificationMutex);

		m_NotificationCv.wait_for(lock, timeout, [&]() { return m_NumNotifications >= expectedCount; });

		boost::test_tools::assertion_result res{m_NumNotifications == expectedCount};
		res.message() << "(" << m_NumNotifications << " == " << expectedCount << ")";

		return res;
	}

	boost::test_tools::assertion_result AssertNoAttemptedSendLogPattern()
	{
		auto result = ExpectLogPattern("^(Sending|Attempting to (re-)?send).*?notification.*$", 0s);
		ClearTestLogger();
		return !result;
	}

	void BeginTimePeriod()
	{
		ObjectLock lock{m_TimePeriod};
		m_TimePeriod->SetRanges(m_AllTimePeriod);

		auto now = Utility::GetTime();
		m_TimePeriod->UpdateRegion(now, now + 1e3, true);
		BOOST_REQUIRE(m_TimePeriod->IsInside(now));
	}

	void EndTimePeriod()
	{
		ObjectLock lock{m_TimePeriod};
		m_TimePeriod->SetRanges(new Dictionary);

		auto now = Utility::GetTime();
		m_TimePeriod->UpdateRegion(now, now + 1e3, true);
		BOOST_REQUIRE(!m_TimePeriod->IsInside(now));
	}

	void SetNotificationInverval(double interval) { m_Notification->SetInterval(interval); }

	void SetNotificationTimes(double begin, double end)
	{
		m_Notification->SetTimes(new Dictionary{{"begin", begin}, {"end", end}});
	}

	void WaitUntilNextReminderScheduled()
	{
		auto now = Utility::GetTime();
		if(now < m_Notification->GetNextNotification()){
			Utility::Sleep(m_Notification->GetNextNotification() - now + 0.01);
		}
		BOOST_REQUIRE_LE(m_Notification->GetNextNotification(), Utility::GetTime());
	}

	static void NotificationTimerHandler()
	{
		auto nc = NotificationComponent::GetByName("nc");
		InvokeTimerHandler(nc);
	}

	void ReceiveCheckResults(std::size_t num, ServiceState state)
	{
		StoppableWaitGroup::Ptr wg = new StoppableWaitGroup();

		for (auto i = 0UL; i < num; ++i) {
			CheckResult::Ptr cr = new CheckResult();

			cr->SetState(state);

			double now = Utility::GetTime();
			cr->SetActive(false);
			cr->SetScheduleStart(now);
			cr->SetScheduleEnd(now);
			cr->SetExecutionStart(now);
			cr->SetExecutionEnd(now);

			BOOST_REQUIRE(m_Host->ProcessCheckResult(cr, wg) == Checkable::ProcessingResult::Ok);
		}
	}

	double GetLastNotificationTimestamp() { return m_Notification->GetLastNotification(); }

	double GetNextNotificationTimestamp() { return m_Notification->GetNextNotification(); }
	void SetNextNotificationTimestamp(double val) { m_Notification->SetNextNotification(val); }

	std::uint8_t GetSuppressedNotifications() { return m_Notification->GetSuppressedNotifications(); }

	static NotificationType GetLastNotification()
	{
		std::unique_lock lock(m_NotificationMutex);
		return m_LastNotification;
	}

	static std::size_t GetNotificationCount()
	{
		std::unique_lock lock(m_NotificationMutex);
		return m_NumNotifications;
	}

private:
	static inline std::mutex m_NotificationMutex;
	static inline std::condition_variable m_NotificationCv;
	static inline std::size_t m_NumNotifications{};
	static inline NotificationType m_LastNotification{};

	Host::Ptr m_Host;
	Notification::Ptr m_Notification;
	TimePeriod::Ptr m_TimePeriod;
	Dictionary::Ptr m_AllTimePeriod;
};

BOOST_FIXTURE_TEST_SUITE(notificationcomponent, NotificationComponentFixture,
	*boost::unit_test::label("notification"));

/* Test sending out reminder notifications in a given interval.
 */
BOOST_AUTO_TEST_CASE(notify_send_reminders)
{
	SetNotificationInverval(0.15);
	ReceiveCheckResults(2, ServiceCritical);

	// The first run of the timer sets up the next reminder notification.
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(1));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);

	// Rerunning the timer before the next interval should not trigger a reminder notification.
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);

	// After waiting until the interval has passed, a reminder will be queued.
	WaitUntilNextReminderScheduled();
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(2));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);

	// Now we test that reminders are only sent for Critical states.
	// Hard state is switched to OK.
	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(3));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationRecovery);

	// Now we wait for one interval and check that no reminder has been sent.
	WaitUntilNextReminderScheduled();
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 3);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationRecovery);
}

/* Tests simple sending of notifications on each state change.
 */
BOOST_AUTO_TEST_CASE(notify_simple)
{
	BeginTimePeriod();

	ReceiveCheckResults(2, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(1));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);

	ReceiveCheckResults(1, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);

	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(2));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationRecovery);

	ReceiveCheckResults(2, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 2);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationRecovery);
}

/* This tests the simplest case where a suppressed notification will be sent after resuming
 * a TimePeriod. A single event occurs outside the TimePeriod and the notification should be
 * sent as soon as the timer runs after the TimePeriod is resumed.
 */
BOOST_AUTO_TEST_CASE(notify_after_timeperiod_simple)
{
	BeginTimePeriod();

	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	EndTimePeriod();

	ReceiveCheckResults(3, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationProblem);

	BeginTimePeriod();
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(1));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);
}

/* Similar to the test-case above, but has multiple state-changes outside of the TimePeriod
 * This is important, since there are multiple places in the code that check on and make modifications
 * to the list of suppressed events. A concrete example of a bug like this is #10575.
 */
BOOST_AUTO_TEST_CASE(notify_multiple_state_changes_outside_timeperiod)
{
	BOOST_REQUIRE_EQUAL(GetLastNotificationTimestamp(), 0.0);

	BeginTimePeriod();
	ReceiveCheckResults(2, ServiceCritical);

	BOOST_REQUIRE(WaitForExpectedNotificationCount(1));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	EndTimePeriod();

	ReceiveCheckResults(1, ServiceOK);
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationRecovery);

	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationRecovery);

	ReceiveCheckResults(1, ServiceCritical);
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationRecovery);

	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationRecovery);

	// Third Critical check result will set the Critical hard state.
	ReceiveCheckResults(2, ServiceCritical);
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	ReceiveCheckResults(1, ServiceOK);
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationRecovery);

	BeginTimePeriod();

	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(2));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationRecovery);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);
}

/* This tests if suppressed notifications of opposite types cancel each other out.
 */
BOOST_AUTO_TEST_CASE(no_notify_suppressed_cancel_out)
{
	BOOST_REQUIRE_EQUAL(GetLastNotificationTimestamp(), 0.0);

	BeginTimePeriod();

	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	EndTimePeriod();

	ReceiveCheckResults(3, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationProblem);

	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	BeginTimePeriod();

	// Ensure no notification is sent after resuming the TimePeriod.
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	// Now repeat the same starting from a Critical state
	ReceiveCheckResults(3, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(WaitForExpectedNotificationCount(1));
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	EndTimePeriod();

	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationRecovery);

	ReceiveCheckResults(3, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	BeginTimePeriod();

	// Ensure no notification is sent after resuming the TimePeriod.
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 1);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), NotificationProblem);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);
}

/* This may look similar to the test-case above, but the critical difference is that here
 * the final state change happens inside the TimePeriod again, but before the timer runs.
 * The outdated suppressed NotificationProblem will then be subtracted when the timer runs.
 */
BOOST_AUTO_TEST_CASE(no_notify_non_applicable_reason)
{
	BOOST_REQUIRE_EQUAL(GetLastNotificationTimestamp(), 0.0);

	BeginTimePeriod();

	ReceiveCheckResults(1, ServiceOK);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);

	EndTimePeriod();

	// We queue a suppressed notification.
	ReceiveCheckResults(3, ServiceCritical);
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationProblem);

	BeginTimePeriod();

	// In this scenario a check result that goes against the suppressed notification is processed
	// before the timer can run again. No notification should be sent, because the last state
	// change the user was notified about was the same.
	ReceiveCheckResults(1, ServiceOK);
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), NotificationProblem);

	// When the timer runs, it should clear the suppressed notification but not send anything.
	NotificationTimerHandler();
	BOOST_REQUIRE(AssertNoAttemptedSendLogPattern());
	BOOST_REQUIRE_EQUAL(GetNotificationCount(), 0);
	BOOST_REQUIRE_EQUAL(GetLastNotification(), 0);
	BOOST_REQUIRE_EQUAL(GetSuppressedNotifications(), 0);
}

/**
 * This tests for regressions of races around the NoMoreNotifications flag, like in #10623.
 *
 * The potential for race-conditions exists in this case because check-results potentially
 * leading to notifications are processed synchronously in SendNotificationsHandler() and
 * asynchronously in NotificationTimerHandler() when the timer runs out.
 */
BOOST_AUTO_TEST_CASE(no_more_notifications_race)
{
	constexpr auto numIterations = 20UL;

	BeginTimePeriod();

	// Run the handler in a loop to provoke any existing race conditions.
	std::atomic_bool stop;
	auto timerThread = std::thread{[&stop]() {
		while (!stop) {
			NotificationTimerHandler();
		}
	}};

	ReceiveCheckResults(1, ServiceOK);

	// With interval 0, no reminder notifications should ever be sent.
	for (auto i = 0UL; i < numIterations; ++i) {
		ReceiveCheckResults(3, ServiceCritical);
		ReceiveCheckResults(1, ServiceOK);
	}

	stop = true;
	timerThread.join();

	BOOST_REQUIRE(WaitForExpectedNotificationCount(2 * numIterations));
	BOOST_REQUIRE(!WaitForExpectedNotificationCount((2 * numIterations) + 1, 10ms));
	BOOST_REQUIRE(!ExpectLogPattern("^Sending reminder.*$", 0s));
}

BOOST_AUTO_TEST_SUITE_END()
