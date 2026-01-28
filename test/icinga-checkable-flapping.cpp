// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/host.hpp"
#include <bitset>
#include <iostream>
#include <BoostTestTargetConfig.h>

using namespace icinga;

#ifdef I2_DEBUG
static CheckResult::Ptr MakeCheckResult(ServiceState state)
{
	CheckResult::Ptr cr = new CheckResult();

	cr->SetState(state);

	double now = Utility::GetTime();
	cr->SetScheduleStart(now);
	cr->SetScheduleEnd(now);
	cr->SetExecutionStart(now);
	cr->SetExecutionEnd(now);

	Utility::IncrementTime(60);

	return cr;
}

static void LogFlapping(const Checkable::Ptr& obj)
{
	std::bitset<20> stateChangeBuf = obj->GetFlappingBuffer();
	int oldestIndex = (obj->GetFlappingBuffer() & 0xFF00000) >> 20;

	std::cout << "Flapping: " << obj->IsFlapping() << "\nHT: " << obj->GetFlappingThresholdHigh() << " LT: "
		<< obj->GetFlappingThresholdLow() << "\nOur value: " << obj->GetFlappingCurrent() << "\nPtr: " << oldestIndex
		<< " Buf: " << stateChangeBuf.to_ulong() << '\n';
}


static void LogHostStatus(const Host::Ptr &host)
{
	std::cout << "Current status: state: " << host->GetState() << " state_type: " << host->GetStateType()
		<< " check attempt: " << host->GetCheckAttempt() << "/" << host->GetMaxCheckAttempts() << " Active: " << host->IsActive() << std::endl;
}
#endif /* I2_DEBUG */

BOOST_AUTO_TEST_SUITE(icinga_checkable_flapping)

BOOST_AUTO_TEST_CASE(host_not_flapping)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	std::cout << "Running test with a non-flapping host...\n";

	Host::Ptr host = new Host();
	host->SetName("test");
	host->SetEnableFlapping(true);
	host->SetMaxCheckAttempts(5);
	host->SetActive(true);

	// Host otherwise is soft down
	host->SetState(HostUp);
	host->SetStateType(StateTypeHard);

	Utility::SetTime(0);

	BOOST_CHECK(host->GetFlappingCurrent() == 0);

	LogFlapping(host);
	LogHostStatus(host);

	// watch the state being stable
	int i = 0;
	while (i++ < 10) {
		// For some reason, elusive to me, the first check is a state change
		host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());

		LogFlapping(host);
		LogHostStatus(host);

		BOOST_CHECK(host->GetState() == 0);
		BOOST_CHECK(host->GetCheckAttempt() == 1);
		BOOST_CHECK(host->GetStateType() == StateTypeHard);

		//Should not be flapping
		BOOST_CHECK(!host->IsFlapping());
		BOOST_CHECK(host->GetFlappingCurrent() < 30.0);
	}
#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(host_flapping)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	std::cout << "Running test with host changing state with every check...\n";

	Host::Ptr host = new Host();
	host->SetName("test");
	host->SetEnableFlapping(true);
	host->SetMaxCheckAttempts(5);
	host->SetActive(true);

	Utility::SetTime(0);

	int i = 0;
	while (i++ < 25) {
		if (i % 2)
			host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
		else
			host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());

		LogFlapping(host);
		LogHostStatus(host);

		//30 Percent is our high Threshold
		if (i >= 6) {
			BOOST_CHECK(host->IsFlapping());
		} else {
			BOOST_CHECK(!host->IsFlapping());
		}
	}
#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(host_flapping_recover)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	std::cout << "Running test with flapping recovery...\n";

	Host::Ptr host = new Host();
	host->SetName("test");
	host->SetEnableFlapping(true);
	host->SetMaxCheckAttempts(5);
	host->SetActive(true);

	// Host otherwise is soft down
	host->SetState(HostUp);
	host->SetStateType(StateTypeHard);

	Utility::SetTime(0);

	// A few warning
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());

	LogFlapping(host);
	LogHostStatus(host);
	for (int i = 0; i <= 7; i++) {
		if (i % 2)
			host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
		else
			host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	}

	LogFlapping(host);
	LogHostStatus(host);

	// We should be flapping now
	BOOST_CHECK(host->GetFlappingCurrent() > 30.0);
	BOOST_CHECK(host->IsFlapping());

	// Now recover from flapping
	int count = 0;
	while (host->IsFlapping()) {
		BOOST_CHECK(host->GetFlappingCurrent() > 25.0);
		BOOST_CHECK(host->IsFlapping());

		host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
		LogFlapping(host);
		LogHostStatus(host);
		count++;
	}

	std::cout << "Recovered from flapping after " << count << " Warning results.\n";

	BOOST_CHECK(host->GetFlappingCurrent() < 25.0);
	BOOST_CHECK(!host->IsFlapping());
#endif /* I2_DEBUG */
}

BOOST_AUTO_TEST_CASE(host_flapping_docs_example)
{
#ifndef I2_DEBUG
	BOOST_WARN_MESSAGE(false, "This test can only be run in a debug build!");
#else /* I2_DEBUG */
	std::cout << "Simulating the documentation example...\n";

	Host::Ptr host = new Host();
	host->SetName("test");
	host->SetEnableFlapping(true);
	host->SetMaxCheckAttempts(5);
	host->SetActive(true);

	// Host otherwise is soft down
	host->SetState(HostUp);
	host->SetStateType(StateTypeHard);

	Utility::SetTime(0);

	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceOK), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceWarning), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());

	LogFlapping(host);
	LogHostStatus(host);
	BOOST_CHECK(host->GetFlappingCurrent() == 39.1);
	BOOST_CHECK(host->IsFlapping());

	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());
	host->ProcessCheckResult(MakeCheckResult(ServiceCritical), new StoppableWaitGroup());

	LogFlapping(host);
	LogHostStatus(host);
	BOOST_CHECK(host->GetFlappingCurrent() < 25.0);
	BOOST_CHECK(!host->IsFlapping());
#endif
}

BOOST_AUTO_TEST_SUITE_END()
