// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "base/application.hpp"
#include "base/loader.hpp"
#include "icingaapplication-fixture.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

struct LivestatusFixture
{
	LivestatusFixture()
	{
		// ensure IcingaApplication is initialized before we try to add config
		IcingaApplicationFixture icinga;

		BOOST_TEST_MESSAGE("Preparing config objects...");

		ConfigItem::RunWithActivationContext(new Function("CreateTestObjects", CreateTestObjects));
	}

	static void CreateTestObjects()
	{
		String config = R"CONFIG(
object CheckCommand "dummy" {
  command = "/bin/echo"
}

object Host "test-01" {
  address = "127.0.0.1"
  check_command = "dummy"
}

object Host "test-02" {
  address = "127.0.0.2"
  check_command = "dummy"
}

apply Service "livestatus" {
  check_command = "dummy"
  notes = "test livestatus"
  assign where match("test-*", host.name)
}
)CONFIG";

		std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<livestatus>", config);
		expr->Evaluate(*ScriptFrame::GetCurrentFrame());
	}
};

BOOST_GLOBAL_FIXTURE(LivestatusFixture);
