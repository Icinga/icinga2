/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga/host.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

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
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	Host::Ptr host = new Host();
	host->SetMaxCheckAttempts(1);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "First check result (unknown)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceUnknown));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Second check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationRecovery);

	std::cout << "Third check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationRecovery);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(host_2attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	Host::Ptr host = new Host();
	host->SetMaxCheckAttempts(2);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "First check result (unknown)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceUnknown));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "Second check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Third check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationRecovery);

	std::cout << "Fourth check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "Fifth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(host_3attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	Host::Ptr host = new Host();
	host->SetMaxCheckAttempts(3);
	host->Activate();
	host->SetAuthority(true);
	host->SetStateRaw(ServiceOK);
	host->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "First check result (unknown)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceUnknown));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "Second check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 2);
	CheckNotification(host, false);

	std::cout << "Third check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, true, NotificationRecovery);

	std::cout << "Fifth check result (critical)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(host->GetState() == HostDown);
	BOOST_CHECK(host->GetStateType() == StateTypeSoft);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	std::cout << "Sixth check result (ok)" << std::endl;
	host->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(host->GetState() == HostUp);
	BOOST_CHECK(host->GetStateType() == StateTypeHard);
	BOOST_CHECK(host->GetCheckAttempt() == 1);
	CheckNotification(host, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(service_1attempt)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	Service::Ptr service = new Service();
	service->SetMaxCheckAttempts(1);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "First check result (unknown)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceUnknown));
	BOOST_CHECK(service->GetState() == ServiceUnknown);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Second check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationRecovery);

	std::cout << "Third check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationRecovery);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(service_2attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	Service::Ptr service = new Service();
	service->SetMaxCheckAttempts(2);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "First check result (unknown)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceUnknown));
	BOOST_CHECK(service->GetState() == ServiceUnknown);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "Second check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Third check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationRecovery);

	std::cout << "Fourth check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "Fifth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(service_3attempts)
{
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	Service::Ptr service = new Service();
	service->SetMaxCheckAttempts(3);
	service->Activate();
	service->SetAuthority(true);
	service->SetStateRaw(ServiceOK);
	service->SetStateType(StateTypeHard);

	std::cout << "Before first check result (ok, hard)" << std::endl;
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "First check result (unknown)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceUnknown));
	BOOST_CHECK(service->GetState() == ServiceUnknown);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "Second check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 2);
	CheckNotification(service, false);

	std::cout << "Third check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationProblem);

	std::cout << "Fourth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, true, NotificationRecovery);

	std::cout << "Fifth check result (critical)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	BOOST_CHECK(service->GetState() == ServiceCritical);
	BOOST_CHECK(service->GetStateType() == StateTypeSoft);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	std::cout << "Sixth check result (ok)" << std::endl;
	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	BOOST_CHECK(service->GetState() == ServiceOK);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetCheckAttempt() == 1);
	CheckNotification(service, false);

	c.disconnect();
}

BOOST_AUTO_TEST_CASE(host_flapping_notification)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	int timeStepInterval = 60;

	Host::Ptr host = new Host();
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
		host->ProcessCheckResult(MakeCheckResult(state));
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(host->IsFlapping() == true);

	CheckNotification(host, true, NotificationFlappingStart);

	std::cout << "Now calm down..." << std::endl;

	for (int i = 0; i < 20; i++) {
		host->ProcessCheckResult(MakeCheckResult(ServiceOK));
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
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
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
		service->ProcessCheckResult(MakeCheckResult(state));
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);



	std::cout << "Now calm down..." << std::endl;

	for (int i = 0; i < 20; i++) {
		service->ProcessCheckResult(MakeCheckResult(ServiceOK));
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	c.disconnect();

#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(service_flapping_problem_notifications)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
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
		service->ProcessCheckResult(MakeCheckResult(state));
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);

	//Insert enough check results to get into hard problem state but staying flapping

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);


	BOOST_CHECK(service->IsFlapping() == true);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	CheckNotification(service, false, NotificationProblem);

	// Calm down
	while (service->IsFlapping()) {
		service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
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

	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
	Utility::IncrementTime(timeStepInterval);

	// Known failure, see #5713
	// CheckNotification(service, true, NotificationRecovery);

	c.disconnect();

#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(service_flapping_ok_into_bad)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
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
		service->ProcessCheckResult(MakeCheckResult(state));
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);

	//Insert enough check results to get into hard problem state but staying flapping

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);


	BOOST_CHECK(service->IsFlapping() == true);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	CheckNotification(service, false, NotificationProblem);

	// Calm down
	while (service->IsFlapping()) {
		service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
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
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	boost::signals2::connection c = Checkable::OnNotificationsRequested.connect(std::bind(&NotificationHandler, _1, _2));

	int timeStepInterval = 60;

	Service::Ptr service = new Service();
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
		service->ProcessCheckResult(MakeCheckResult(state));
		Utility::IncrementTime(timeStepInterval);
	}

	BOOST_CHECK(service->IsFlapping() == true);

	CheckNotification(service, true, NotificationFlappingStart);

	//Insert enough check results to get into hard problem state but staying flapping

	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);
	service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
	Utility::IncrementTime(timeStepInterval);


	BOOST_CHECK(service->IsFlapping() == true);
	BOOST_CHECK(service->GetStateType() == StateTypeHard);
	BOOST_CHECK(service->GetState() == ServiceCritical);

	CheckNotification(service, false, NotificationProblem);

	// Calm down
	while (service->IsFlapping()) {
		service->ProcessCheckResult(MakeCheckResult(ServiceCritical));
		Utility::IncrementTime(timeStepInterval);
	}

	CheckNotification(service, true, NotificationFlappingEnd);

	service->ProcessCheckResult(MakeCheckResult(ServiceOK));
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
BOOST_AUTO_TEST_SUITE_END()
