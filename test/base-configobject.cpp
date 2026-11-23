// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/configobject.hpp"
#include <boost/signals2/connection.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace icinga;

namespace
{
using Events = std::vector<std::string>;
using Signals = std::vector<std::string>;

class TestConfigObject final : public ConfigObject
{
public:
	DECLARE_PTR_TYPEDEFS(TestConfigObject);

	explicit TestConfigObject(HAMode haMode = HARunOnce)
	{
		SetName("test");
		SetHAMode(haMode);
	}

	void Start(bool runtimeCreated) override
	{
		Transition("Start", "Stopped", "Started");
		ConfigObject::Start(runtimeCreated);
	}

	void Resume() override
	{
		if (!IsActive()) {
			m_Violations.emplace_back("Resume() called on inactive object");
		}

		if (std::exchange(m_ThrowOnResume, false)) {
			m_Events.emplace_back("Resume (threw)");
			throw std::runtime_error("Resume failed");
		}

		Transition("Resume", "Started", "Running");
		ConfigObject::Resume();
	}

	void Pause() override
	{
		Transition("Pause", "Running", "Started");
		ConfigObject::Pause();
	}

	void Stop(bool runtimeRemoved) override
	{
		Transition("Stop", "Started", "Stopped");
		ConfigObject::Stop(runtimeRemoved);
	}

	Events GetEvents() const { return m_Events; }

	std::vector<std::string> GetViolations() const { return m_Violations; }

	std::string GetState() const { return m_State; }

	void ThrowOnNextResume() { m_ThrowOnResume = true; }

private:
	void Transition(const std::string& event, std::string from, std::string to)
	{
		m_Events.emplace_back(event);

		if (m_State != from) {
			m_Violations.emplace_back(event + "() called in state " + m_State + ", expected " + std::move(from));
		}

		m_State = std::move(to);
	}

	std::string m_State{"Stopped"};
	Events m_Events;
	std::vector<std::string> m_Violations;
	bool m_ThrowOnResume{};
};
} // namespace

static void CheckLifecycleConsistent(const TestConfigObject::Ptr& obj)
{
	for (auto& violation : obj->GetViolations()) {
		BOOST_ERROR("Lifecycle violation: " << violation);
	}

	BOOST_CHECK_EQUAL(obj->IsPaused(), obj->GetState() != "Running");
}

template<typename F>
static Signals ObserveActiveChanged(const TestConfigObject::Ptr& obj, F&& fn)
{
	Signals observed;

	boost::signals2::scoped_connection conn = ConfigObject::OnActiveChanged.connect(
		[&](const ConfigObject::Ptr& changed, const Value&) {
			if (changed == obj) {
				observed.emplace_back((obj->IsActive() ? "active/" : "inactive/") + obj->GetState());
			}
		}
	);

	std::forward<F>(fn)();

	return observed;
}

BOOST_AUTO_TEST_SUITE(base_configobject)

// NOLINTBEGIN(readability-identifier-naming)
// NOLINTBEGIN(misc-use-internal-linkage)

BOOST_AUTO_TEST_CASE(initial_state)
{
	TestConfigObject::Ptr obj = new TestConfigObject();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_CHECK(obj->GetState() == "Stopped");
	BOOST_CHECK(obj->GetEvents().empty());
}

BOOST_AUTO_TEST_CASE(activate_run_once)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->PreActivate();
	BOOST_CHECK(obj->IsActive());
	BOOST_CHECK(obj->GetEvents().empty());

	obj->Activate();

	BOOST_CHECK(obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(activate_run_everywhere)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);

	obj->PreActivate();
	obj->Activate();

	BOOST_CHECK(obj->IsActive());
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(activate_is_idempotent)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);

	auto observed = ObserveActiveChanged(obj, [&] {
		obj->PreActivate();
		obj->Activate();
		obj->Activate();
	});

	BOOST_TEST(observed == Signals{"active/Running"}, boost::test_tools::per_element());
	BOOST_CHECK(obj->IsActive());
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(activate_without_preactivate)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);

	obj->Activate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_CHECK(obj->GetState() == "Stopped");
	BOOST_CHECK(obj->GetEvents().empty());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(authority_grant_and_revoke)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->PreActivate();
	obj->Activate();

	obj->SetAuthority(true);
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume"}), boost::test_tools::per_element());

	obj->SetAuthority(true);
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume"}), boost::test_tools::per_element());

	obj->SetAuthority(false);
	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume", "Pause"}), boost::test_tools::per_element());

	obj->SetAuthority(false);
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume", "Pause"}), boost::test_tools::per_element());

	obj->SetAuthority(true);
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume", "Pause", "Resume"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(revoke_authority_on_paused_object)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->PreActivate();
	obj->Activate();

	obj->SetAuthority(false);

	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(deactivate_running)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);
	obj->PreActivate();
	obj->Activate();
	obj->SetAuthority(true);

	obj->Deactivate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume", "Pause", "Stop"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(deactivate_paused)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);
	obj->PreActivate();
	obj->Activate();

	obj->Deactivate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Stop"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(deactivate_run_everywhere)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);
	obj->PreActivate();
	obj->Activate();

	obj->Deactivate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume", "Pause", "Stop"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(deactivate_is_idempotent)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);
	obj->PreActivate();
	obj->Activate();

	obj->Deactivate();
	obj->Deactivate();

	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Stop"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(deactivate_never_activated)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->Deactivate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->GetEvents().empty());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(reactivate_after_deactivate)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);
	obj->PreActivate();
	obj->Activate();
	obj->SetAuthority(true);
	obj->Deactivate();

	obj->PreActivate();
	obj->Activate();
	BOOST_CHECK(obj->IsActive());
	BOOST_CHECK(obj->IsPaused());

	obj->SetAuthority(true);
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(
		obj->GetEvents() == (Events{"Start", "Resume", "Pause", "Stop", "Start", "Resume"}),
		boost::test_tools::per_element()
	);
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(active_changed_signal_run_once)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	auto observed = ObserveActiveChanged(obj, [&] {
		obj->PreActivate();
		obj->Activate();
		obj->Deactivate();
	});

	BOOST_TEST(observed == (Signals{"active/Started", "inactive/Stopped"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(active_changed_signal_run_everywhere)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);

	auto observed = ObserveActiveChanged(obj, [&] {
		obj->PreActivate();
		obj->Activate();
		obj->Deactivate();
	});

	BOOST_TEST(observed == (Signals{"active/Running", "inactive/Stopped"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(active_changed_signal_not_started)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);

	auto observed = ObserveActiveChanged(obj, [&] {
		obj->PreActivate();
		obj->Deactivate();
	});

	BOOST_TEST(observed == Signals{"inactive/Stopped"}, boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(authority_on_never_activated_object)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->SetAuthority(true);

	BOOST_CHECK(obj->IsPaused());
	BOOST_CHECK(obj->GetEvents().empty());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(authority_between_preactivate_and_activate)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->PreActivate();
	obj->SetAuthority(true);

	BOOST_CHECK(obj->IsPaused());
	BOOST_CHECK(obj->GetEvents().empty());
	CheckLifecycleConsistent(obj);

	obj->Activate();
	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);

	obj->SetAuthority(true);
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(authority_after_deactivate)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);
	obj->PreActivate();
	obj->Activate();
	obj->SetAuthority(true);
	obj->Deactivate();

	obj->SetAuthority(true);

	BOOST_CHECK(obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume", "Pause", "Stop"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(deactivate_between_preactivate_and_activate)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);

	obj->PreActivate();
	obj->Deactivate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->GetEvents().empty());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(activate_after_deactivate_between_preactivate_and_activate)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunEverywhere);

	obj->PreActivate();
	obj->Deactivate();
	obj->Activate();

	BOOST_CHECK(!obj->IsActive());
	BOOST_CHECK(obj->IsPaused());
	BOOST_CHECK(obj->GetState() == "Stopped");
	BOOST_CHECK(obj->GetEvents().empty());
	CheckLifecycleConsistent(obj);
}

BOOST_AUTO_TEST_CASE(resume_throws)
{
	TestConfigObject::Ptr obj = new TestConfigObject(HARunOnce);
	obj->PreActivate();
	obj->Activate();

	obj->ThrowOnNextResume();
	BOOST_CHECK_THROW(obj->SetAuthority(true), std::runtime_error);
	BOOST_CHECK(obj->IsPaused());
	CheckLifecycleConsistent(obj);

	obj->SetAuthority(true);
	BOOST_CHECK(!obj->IsPaused());
	BOOST_TEST(obj->GetEvents() == (Events{"Start", "Resume (threw)", "Resume"}), boost::test_tools::per_element());
	CheckLifecycleConsistent(obj);
}

// NOLINTEND(misc-use-internal-linkage)
// NOLINTEND(readability-identifier-naming)

BOOST_AUTO_TEST_SUITE_END()
