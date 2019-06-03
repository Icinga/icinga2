/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/notification.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_notification)

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
BOOST_AUTO_TEST_SUITE_END()
