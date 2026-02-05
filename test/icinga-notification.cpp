// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/host.hpp"
#include "icinga/notification.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/service.hpp"
#include "icinga/user.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

struct DuplicateDueToFilterHelper
{
	Host::Ptr h = new Host();
	Service::Ptr s = new Service();
	User::Ptr u = new User();
	NotificationCommand::Ptr nc = new NotificationCommand();
	Notification::Ptr n = new Notification();
	unsigned int called = 0;

	DuplicateDueToFilterHelper(int typeFilter, int stateFilter)
	{
		h->SetName("example.com", true);
		h->Register();

		s->SetShortName("disk", true);
		h->AddService(s);

		u->SetName("jdoe", true);
		u->SetTypeFilter(~0);
		u->SetStateFilter(~0);
		u->Register();

		nc->SetName("mail", true);
		nc->SetExecute(new Function("", [this]() { ++called; }), true);
		nc->Register();

		n->SetFieldByName("host_name", "example.com", DebugInfo());
		n->SetFieldByName("service_name", "disk", DebugInfo());
		n->SetFieldByName("command", "mail", DebugInfo());
		n->SetUsersRaw(new Array({"jdoe"}), true);
		n->SetTypeFilter(typeFilter);
		n->SetStateFilter(stateFilter);
		n->OnAllConfigLoaded(); // link Service
	}

	~DuplicateDueToFilterHelper()
	{
		h->Unregister();
		u->Unregister();
		nc->Unregister();
	}

	void SendStateNotification(ServiceState state, bool isSent)
	{
		auto calledBefore (called);

		s->SetStateRaw(state, true);
		Application::GetTP().Start();

		n->BeginExecuteNotification(
			state == ServiceOK ? NotificationRecovery : NotificationProblem,
			nullptr, false, false, "", ""
		);

		Application::GetTP().Stop();
		BOOST_CHECK_EQUAL(called > calledBefore, isSent);
	}
};

BOOST_AUTO_TEST_SUITE(icinga_notification)

BOOST_AUTO_TEST_CASE(strings)
{
	// States
	BOOST_CHECK("OK" == Notification::NotificationServiceStateToString(ServiceOK));
	BOOST_CHECK("Critical" == Notification::NotificationServiceStateToString(ServiceCritical));
	BOOST_CHECK("Up" == Notification::NotificationHostStateToString(HostUp));

	// Types
	BOOST_CHECK("DowntimeStart" == Notification::NotificationTypeToString(NotificationDowntimeStart));
	BOOST_CHECK("Problem" == Notification::NotificationTypeToString(NotificationProblem));

	// Compat
	BOOST_CHECK("DOWNTIMECANCELLED" == Notification::NotificationTypeToStringCompat(NotificationDowntimeRemoved));
}

BOOST_AUTO_TEST_CASE(state_filter)
{
	unsigned long fstate;

	Array::Ptr states = new Array();
	states->Add("OK");
	states->Add("Warning");

	Notification::Ptr notification = new Notification();

	notification->SetStateFilter(FilterArrayToInt(states, notification->GetStateFilterMap(), ~0));
	notification->Activate();
	notification->SetAuthority(true);

	/* Test passing notification state */
	fstate = StateFilterWarning;
	std::cout << "#1 Notification state: " << fstate << " against " << notification->GetStateFilter() << " must pass. " << std::endl;
	BOOST_CHECK(notification->GetStateFilter() & fstate);

	/* Test filtered notification state */
	fstate = StateFilterUnknown;
	std::cout << "#2 Notification state: " << fstate << " against " << notification->GetStateFilter() << " must fail." << std::endl;
	BOOST_CHECK(!(notification->GetStateFilter() & fstate));

	/* Test unset states filter configuration */
	notification->SetStateFilter(FilterArrayToInt(Array::Ptr(), notification->GetStateFilterMap(), ~0));

	fstate = StateFilterOK;
	std::cout << "#3 Notification state: " << fstate << " against " << notification->GetStateFilter() << " must pass." << std::endl;
	BOOST_CHECK(notification->GetStateFilter() & fstate);

	/* Test empty states filter configuration */
	states->Clear();
	notification->SetStateFilter(FilterArrayToInt(states, notification->GetStateFilterMap(), ~0));

	fstate = StateFilterCritical;
	std::cout << "#4 Notification state: " << fstate << " against " << notification->GetStateFilter() << " must fail." << std::endl;
	BOOST_CHECK(!(notification->GetStateFilter() & fstate));
}
BOOST_AUTO_TEST_CASE(type_filter)
{
	unsigned long ftype;

	Array::Ptr types = new Array();
	types->Add("Problem");
	types->Add("DowntimeStart");
	types->Add("DowntimeEnd");

	Notification::Ptr notification = new Notification();

	notification->SetTypeFilter(FilterArrayToInt(types, notification->GetTypeFilterMap(), ~0));
	notification->Activate();
	notification->SetAuthority(true);

	/* Test passing notification type */
	ftype = NotificationProblem;
	std::cout << "#1 Notification type: " << ftype << " against " << notification->GetTypeFilter() << " must pass." << std::endl;
	BOOST_CHECK(notification->GetTypeFilter() & ftype);

	/* Test filtered notification type */
	ftype = NotificationCustom;
	std::cout << "#2 Notification type: " << ftype << " against " << notification->GetTypeFilter() << " must fail." << std::endl;
	BOOST_CHECK(!(notification->GetTypeFilter() & ftype));

	/* Test unset types filter configuration */
	notification->SetTypeFilter(FilterArrayToInt(Array::Ptr(), notification->GetTypeFilterMap(), ~0));

	ftype = NotificationRecovery;
	std::cout << "#3 Notification type: " << ftype << " against " << notification->GetTypeFilter() << " must pass." << std::endl;
	BOOST_CHECK(notification->GetTypeFilter() & ftype);

	/* Test empty types filter configuration */
	types->Clear();
	notification->SetTypeFilter(FilterArrayToInt(types, notification->GetTypeFilterMap(), ~0));

	ftype = NotificationProblem;
	std::cout << "#4 Notification type: " << ftype << " against " << notification->GetTypeFilter() << " must fail." << std::endl;
	BOOST_CHECK(!(notification->GetTypeFilter() & ftype));
}

BOOST_AUTO_TEST_CASE(no_filter_problem_no_duplicate)
{
	DuplicateDueToFilterHelper helper (~0, ~0);

	helper.SendStateNotification(ServiceCritical, true);
	helper.SendStateNotification(ServiceWarning, true);
	helper.SendStateNotification(ServiceCritical, true);
}

BOOST_AUTO_TEST_CASE(filter_problem_no_duplicate)
{
	DuplicateDueToFilterHelper helper (~0, ~StateFilterWarning);

	helper.SendStateNotification(ServiceCritical, true);
	helper.SendStateNotification(ServiceWarning, false);
	helper.SendStateNotification(ServiceCritical, false);
}

BOOST_AUTO_TEST_CASE(volatile_filter_problem_duplicate)
{
	DuplicateDueToFilterHelper helper (~0, ~StateFilterWarning);

	helper.s->SetVolatile(true, true);
	helper.SendStateNotification(ServiceCritical, true);
	helper.SendStateNotification(ServiceWarning, false);
	helper.SendStateNotification(ServiceCritical, true);
}

BOOST_AUTO_TEST_CASE(no_recovery_filter_no_duplicate)
{
	DuplicateDueToFilterHelper helper (~0, ~0);

	helper.SendStateNotification(ServiceCritical, true);
	helper.SendStateNotification(ServiceOK, true);
	helper.SendStateNotification(ServiceCritical, true);
}

BOOST_AUTO_TEST_CASE(recovery_filter_duplicate)
{
	DuplicateDueToFilterHelper helper (~NotificationRecovery, ~0);

	helper.SendStateNotification(ServiceCritical, true);
	helper.SendStateNotification(ServiceOK, false);
	helper.SendStateNotification(ServiceCritical, true);
}

BOOST_AUTO_TEST_SUITE_END()
