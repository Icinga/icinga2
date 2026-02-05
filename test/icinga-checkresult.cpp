// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/downtime.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_checkresult)

static CheckResult::Ptr MakeCheckResult(ServiceState state)
{
	CheckResult::Ptr cr = new CheckResult();

	cr->SetState(state);

	double now = Utility::GetTime();
	cr->SetScheduleStart(now);
	cr->SetScheduleEnd(now);
	cr->SetExecutionStart(now);
	cr->SetExecutionEnd(now);

	return cr;
}

static void NotificationHandler(const Checkable::Ptr& checkable, NotificationType type)
{
	std::cout << "Notification triggered: " << Notification::NotificationTypeToString(type) << std::endl;

	checkable->SetExtension("requested_notifications", true);
	checkable->SetExtension("notification_type", type);
}

static void CheckNotification(const Checkable::Ptr& checkable, bool expected, NotificationType type = NotificationRecovery)
{
	BOOST_CHECK((expected && checkable->GetExtension("requested_notifications").ToBool()) || (!expected && !checkable->GetExtension("requested_notifications").ToBool()));

	if (expected && checkable->GetExtension("requested_notifications").ToBool())
		BOOST_CHECK(checkable->GetExtension("notification_type") == type);

	checkable->SetExtension("requested_notifications", false);
}

BOOST_AUTO_TEST_CASE(host_1attempt)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	Host::Ptr host = new Host();
	host->SetActive(true);
	host->SetMaxCheckAttempts(1);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "First check result (unknown)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceUnknown), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Second check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationRecovery);

	std::cout << "Third check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationRecovery);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(host_2attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	Host::Ptr host = new Host();
	host->SetActive(true);
	host->SetMaxCheckAttempts(2);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "First check result (unknown)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceUnknown), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "Second check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Third check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationRecovery);

	std::cout << "Fourth check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "Fifth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(host_3attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	Host::Ptr host = new Host();
	host->SetActive(true);
	host->SetMaxCheckAttempts(3);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "First check result (unknown)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceUnknown), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "Second check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 2);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "Third check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, true, NotificationRecovery);

	std::cout << "Fifth check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	std::cout << "Sixth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	BOOST_CHECK(host->IsReachable() == true);
	CheckNotification(host, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(service_1attempt)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->SetMaxCheckAttempts(1);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "First check result (unknown)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceUnknown), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceUnknown);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Second check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationRecovery);

	std::cout << "Third check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationRecovery);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(service_2attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->SetMaxCheckAttempts(2);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "First check result (unknown)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceUnknown), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceUnknown);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "Second check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Third check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationRecovery);

	std::cout << "Fourth check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "Fifth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(service_3attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->SetMaxCheckAttempts(3);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "First check result (unknown)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceUnknown), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceUnknown);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "Second check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 2);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "Third check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, true, NotificationRecovery);

	std::cout << "Fifth check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	std::cout << "Sixth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	BOOST_CHECK(service->IsReachable() == true);
	CheckNotification(service, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(host_flapping_notification)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	int timeStepInterval = 60;

	Host::Ptr host = new Host();
	host->SetActive(true);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);
	host->SetEnableFlapping(true);

	/* Initialize start time */
	Utility::SetTime(0);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);

	Utility::IncrementTime(timeStepInterval);

	std::cout << "Inserting flapping check results" << std::endl;

	for (int i = 0; i < 10; i++) {
		ServiceState state = (i % 2 == 0 ? ServiceOK : ServiceCritical);
		host->ProcessCheckResult(MakeCheckResult(state), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(host->IsFlapping() == true);

	CheckNotification(host, true, NotificationFlappingStart);

	std::cout << "Now calm down..." << std::endl;

	for (int i = 0; i < 20; i++) {
		host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(host, true, NotificationFlappingEnd);


	c.disconnect();

#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(service_flapping_notification)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);
	service->SetEnableFlapping(true);

	/* Initialize start time */
	Utility::SetTime(0);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);

	Utility::IncrementTime(timeStepInterval);

	std::cout << "Inserting flapping check results" << std::endl;

	for (int i = 0; i < 10; i++) {
		ServiceState state = (i % 2 == 0 ? ServiceOK : ServiceCritical);
		service->ProcessCheckResult(MakeCheckResult(state), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);



	std::cout << "Now calm down..." << std::endl;

	for (int i = 0; i < 20; i++) {
		service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	c.disconnect();

#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(service_flapping_problem_notifications)
{
#ifndef I2_DEBUG
	BOOST_TEST_MESSAGE("This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);
	service->SetEnableFlapping(true);
	service->SetMaxCheckAttempts(3);

	/* Initialize start time */
	Utility::SetTime(0);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);

	Utility::IncrementTime(timeStepInterval);

	std::cout << "Inserting flapping check results" << std::endl;

	for (int i = 0; i < 10; i++) {
		ServiceState state = (i % 2 == 0 ? ServiceOK : ServiceCritical);
		service->ProcessCheckResult(MakeCheckResult(state), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);

	//Insert enough check results to get into hard problem state but staying flapping

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);


	BOOST_CHECK(service->IsFlapping() == true);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	CheckNotification(service, false, NotificationProblem);

	// Calm down
	while (service->IsFlapping()) {
		service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	/* Intended behaviour is a Problem notification being sent as well, but there are is a Problem:
	 * We don't know whether the Object was Critical before we started flapping and sent out a Notification.
	 * A notification will not be sent, no matter how many criticals follow.
	 *
	 * service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	 * CheckNotification(service, true, NotificationProblem);
	 * ^ This fails, no notification will be sent
	 *
	 * There is also a different issue, when we receive a OK check result, a Recovery Notification will be sent
	 * since the service went from hard critical into soft ok. Yet there is no fitting critical notification.
	 * This should not happen:
	 *
	 * service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	 * CheckNotification(service, false, NotificationRecovery);
	 * ^ This fails, recovery is sent
	 */

	BOOST_CHECK(service->IsFlapping() == false);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	// Known failure, see #5713
	// CheckNotification(service, true, NotificationProblem);

	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);

	// Known failure, see #5713
	// CheckNotification(service, true, NotificationRecovery);

	c.disconnect();

#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(service_flapping_ok_into_bad)
{
#ifndef I2_DEBUG
	BOOST_TEST_MESSAGE("This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);
	service->SetEnableFlapping(true);
	service->SetMaxCheckAttempts(3);

	/* Initialize start time */
	Utility::SetTime(0);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);

	Utility::IncrementTime(timeStepInterval);

	std::cout << "Inserting flapping check results" << std::endl;

	for (int i = 0; i < 10; i++) {
		ServiceState state = (i % 2 == 0 ? ServiceOK : ServiceCritical);
		service->ProcessCheckResult(MakeCheckResult(state), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);

	//Insert enough check results to get into hard problem state but staying flapping

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);


	BOOST_CHECK(service->IsFlapping() == true);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	CheckNotification(service, false, NotificationProblem);

	// Calm down
	while (service->IsFlapping()) {
		service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);

	BOOST_CHECK(service->IsFlapping() == false);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	// We expect a problem notification here
	// Known failure, see #5713
	// CheckNotification(service, true, NotificationProblem);

	c.disconnect();

#endif /* I2_DEBUG */
}
BOOST_AUTO_TEST_CASE(service_flapping_ok_over_bad_into_ok)
{
#ifndef I2_DEBUG
	BOOST_TEST_MESSAGE("This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect([](const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&) {
		NotificationHandler(checkable, type);
	});

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
	service->SetActive(true);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);
	service->SetEnableFlapping(true);
	service->SetMaxCheckAttempts(3);

	/* Initialize start time */
	Utility::SetTime(0);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);

	Utility::IncrementTime(timeStepInterval);

	std::cout << "Inserting flapping check results" << std::endl;

	for (int i = 0; i < 10; i++) {
		ServiceState state = (i % 2 == 0 ? ServiceOK : ServiceCritical);
		service->ProcessCheckResult(MakeCheckResult(state), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);

	//Insert enough check results to get into hard problem state but staying flapping

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);


	BOOST_CHECK(service->IsFlapping() == true);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	CheckNotification(service, false, NotificationProblem);

	// Calm down
	while (service->IsFlapping()) {
		service->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	service->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	Utility::IncrementTime(timeStepInterval);

	BOOST_CHECK(service->IsFlapping() == false);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceOK);

	// There should be no recovery
	// Known failure, see #5713
	// CheckNotification(service, false, NotificationRecovery);

	c.disconnect();

#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(suppressed_notification)
{
	/* Tests that suppressed notifications on a Checkable are sent after the suppression ends if and only if the first
	 * hard state after the suppression is different from the last hard state before the suppression. The test works
	 * by bringing a service in a defined hard state, creating a downtime, performing some state changes, removing the
	 * downtime, bringing the service into another defined hard state (if not already) and checking the requested
	 * notifications.
	 */

	struct NotificationLog {
		std::vector<std::pair<NotificationType, ServiceState>> GetAndClear() {
			std::lock_guard<std::mutex> lock (mutex);

			std::vector<std::pair<NotificationType, ServiceState>> ret;
			std::swap(ret, log);
			return ret;
		}

		void Add(std::pair<NotificationType, ServiceState> notification) {
			std::lock_guard<std::mutex> lock (mutex);

			log.emplace_back(notification);
		}

	private:
		std::mutex mutex;
		std::vector<std::pair<NotificationType, ServiceState>> log;
	};

	const std::vector<ServiceState> states {ServiceOK, ServiceWarning, ServiceCritical, ServiceUnknown};

	for (bool isVolatile : {false, true}) {
		for (int checkAttempts : {1, 2}) {
			for (ServiceState initialState : states) {
				for (ServiceState s1 : states)
				for (ServiceState s2 : states)
				for (ServiceState s3 : states) {
					const std::vector<ServiceState> sequence {s1, s2, s3};

					std::string testcase;

					{
						std::ostringstream buf;
						buf << "volatile=" << isVolatile
							<< " checkAttempts=" << checkAttempts
							<< " sequence={" << Service::StateToString(initialState);

						for (ServiceState s : sequence) {
							buf << " " << Service::StateToString(s);
						}

						buf << "}";
						testcase = buf.str();
					}

					std::cout << "Test case: " << testcase << std::endl;

					// Create host and service for the test.
					Host::Ptr host = new Host();
					host->SetName("suppressed_notifications");
					host->Register();

					Service::Ptr service = new Service();
					service->SetHostName(host->GetName());
					service->SetName("service");
					service->SetActive(true);
					service->SetVolatile(isVolatile);
					service->SetMaxCheckAttempts(checkAttempts);
					service->Activate();
					service->SetAuthority(true);
					service->Register();

					host->OnAllConfigLoaded();
					service->OnAllConfigLoaded();

					// Bring service into the initial hard state.
					for (int i = 0; i < checkAttempts; i++) {
						std::cout << "  ProcessCheckResult("
							<< Service::StateToString(initialState) << ")" << std::endl;
						service->ProcessCheckResult(MakeCheckResult(initialState), new StoppableWaitGroup());
					}

					BOOST_CHECK(service->GetState() == initialState);
					BOOST_CHECK(service->GetStateType() == StateTypeHard);

					/* Keep track of all notifications requested from now on.
					 *
					 * Boost.Signal2 handler may still be executing from another thread after they were disconnected.
					 * Make the structures accessed by the handlers shared pointers so that they remain valid as long
					 * as they may be accessed from one of these handlers.
					 */
					auto notificationLog = std::make_shared<NotificationLog>();

					boost::signals2::scoped_connection c (Checkable::OnNotificationsRequested.connect(
						[notificationLog,service](
							const Checkable::Ptr& checkable, NotificationType type,	const CheckResult::Ptr& cr,
							const String&, const String&, const MessageOrigin::Ptr&
						) {
							BOOST_CHECK_EQUAL(checkable, service);
							std::cout << "  -> OnNotificationsRequested(" << Notification::NotificationTypeToString(type)
								<< ", " << Service::StateToString(cr->GetState()) << ")" << std::endl;

							notificationLog->Add({type, cr->GetState()});
						}
					));

					// Helper to assert which notifications were requested. Implicitly clears the stored notifications.
					auto assertNotifications = [notificationLog](
						const std::vector<std::pair<NotificationType, ServiceState>>& expected,
						const std::string& extraMessage
					) {
						// Pretty-printer for the vectors of requested and expected notifications.
						auto pretty = [](const std::vector<std::pair<NotificationType, ServiceState>>& vec) {
							std::ostringstream s;

							s << "{";
							bool first = true;
							for (const auto &v : vec) {
								if (first) {
									first = false;
								} else {
									s << ", ";
								}
								s << Notification::NotificationTypeToString(v.first)
								  << "/" << Service::StateToString(v.second);
							}
							s << "}";

							return s.str();
						};

						auto got (notificationLog->GetAndClear());

						BOOST_CHECK_MESSAGE(got == expected, "expected=" << pretty(expected)
							<< " got=" << pretty(got)
							<< (extraMessage.empty() ? "" : " ") << extraMessage);
					};

					// Start a downtime for the service.
					std::cout << "  Downtime Start" << std::endl;
					Downtime::Ptr downtime = new Downtime();
					downtime->SetHostName(host->GetName());
					downtime->SetServiceName(service->GetName());
					downtime->SetName("downtime");
					downtime->SetFixed(true);
					downtime->SetStartTime(Utility::GetTime() - 3600);
					downtime->SetEndTime(Utility::GetTime() + 3600);
					service->RegisterDowntime(downtime);
					downtime->Register();
					downtime->OnAllConfigLoaded();
					downtime->TriggerDowntime(Utility::GetTime());

					BOOST_CHECK(service->IsInDowntime());

					// Process check results for the state sequence.
					for (ServiceState s : sequence) {
						std::cout << "  ProcessCheckResult(" << Service::StateToString(s) << ")" << std::endl;
						service->ProcessCheckResult(MakeCheckResult(s), new StoppableWaitGroup());
						BOOST_CHECK(service->GetState() == s);
						if (checkAttempts == 1) {
							BOOST_CHECK(service->GetStateType() == StateTypeHard);
						}
					}

					assertNotifications({}, "(no notifications in downtime)");

					if (service->GetSuppressedNotifications()) {
						BOOST_CHECK_EQUAL(service->GetStateBeforeSuppression(), initialState);
					}

					// Remove the downtime.
					std::cout << "  Downtime End" << std::endl;
					service->UnregisterDowntime(downtime);
					downtime->Unregister();
					BOOST_CHECK(!service->IsInDowntime());

					if (service->GetStateType() == icinga::StateTypeSoft) {
						// When the current state is a soft state, no notification should be sent just yet.
						std::cout << "  FireSuppressedNotifications()" << std::endl;
						service->FireSuppressedNotifications();

						assertNotifications({}, testcase + " (should not fire in soft state)");

						// Repeat the last check result until reaching a hard state.
						for (int i = 0; i < checkAttempts && service->GetStateType() == StateTypeSoft; i++) {
							std::cout << "  ProcessCheckResult(" << Service::StateToString(sequence.back()) << ")"
									  << std::endl;
							service->ProcessCheckResult(MakeCheckResult(sequence.back()), new StoppableWaitGroup());
							BOOST_CHECK(service->GetState() == sequence.back());
						}
					}

					// The service should be in a hard state now and notifications should now be sent if applicable.
					BOOST_CHECK(service->GetStateType() == StateTypeHard);

					std::cout << "  FireSuppressedNotifications()" << std::endl;
					service->FireSuppressedNotifications();

					if (initialState != sequence.back()) {
						NotificationType t = sequence.back() == ServiceOK ? NotificationRecovery : NotificationProblem;
						assertNotifications({{t, sequence.back()}}, testcase);
					} else {
						assertNotifications({}, testcase);
					}

					// Remove host and service.
					service->Unregister();
					host->Unregister();
				}
			}
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
