// SPDX-FileCopyrightText: 2023 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/array.hpp"
#include "icinga/checkresult.hpp"
#include "icinga/host.hpp"
#include "icinga/notification.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/service.hpp"
#include "icinga/user.hpp"
#include "methods/pluginnotificationtask.hpp"
#include <BoostTestTargetConfig.h>
#include <future>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(methods_pluginnotificationtask)

BOOST_AUTO_TEST_CASE(truncate_long_output)
{
#ifdef __linux__
	Host::Ptr h = new Host();
	CheckResult::Ptr hcr = new CheckResult();
	CheckResult::Ptr scr = new CheckResult();
	Service::Ptr s = new Service();
	User::Ptr u = new User();
	NotificationCommand::Ptr nc = new NotificationCommand();
	Notification::Ptr n = new Notification();
	String placeHolder (1024 * 1024, 'x');
	std::promise<String> promise;
	auto future (promise.get_future());

	hcr->SetOutput("H" + placeHolder + "h", true);
	scr->SetOutput("S" + placeHolder + "s", true);

	h->SetName("example.com", true);
	h->SetLastCheckResult(hcr, true);
	h->Register();

	s->SetHostName("example.com", true);
	s->SetShortName("disk", true);
	s->SetLastCheckResult(scr, true);
	s->OnAllConfigLoaded(); // link Host

	nc->SetCommandLine(
		new Array({
			"echo",
			"host_output=$host.output$",
			"service_output=$service.output$",
			"notification_comment=$notification.comment$",
			"output=$output$",
			"comment=$comment$"
		}),
		true
	);

	nc->SetName("mail", true);
	nc->Register();

	n->SetFieldByName("host_name", "example.com", DebugInfo());
	n->SetFieldByName("service_name", "disk", DebugInfo());
	n->SetFieldByName("command", "mail", DebugInfo());
	n->OnAllConfigLoaded(); // link Service

	Checkable::ExecuteCommandProcessFinishedHandler = [&promise](const Value&, const ProcessResult& pr) {
		promise.set_value(pr.Output);
	};

	PluginNotificationTask::ScriptFunc(n, u, nullptr, NotificationCustom, "jdoe", "C" + placeHolder + "c", nullptr, false);
	future.wait();

	Checkable::ExecuteCommandProcessFinishedHandler = nullptr;
	h->Unregister();
	nc->Unregister();

	auto output (future.get());

	BOOST_CHECK(output.Contains("host_output=Hx"));
	BOOST_CHECK(!output.Contains("xh"));
	BOOST_CHECK(output.Contains("x service_output=Sx"));
	BOOST_CHECK(!output.Contains("xs"));
	BOOST_CHECK(output.Contains("x notification_comment=Cx"));
	BOOST_CHECK(!output.Contains("xc"));
	BOOST_CHECK(output.Contains("x output=Sx"));
	BOOST_CHECK(output.Contains("x comment=Cx"));
#endif /* __linux__ */
}

BOOST_AUTO_TEST_SUITE_END()
