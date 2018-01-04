/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "base/application.hpp"
#include "base/loader.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

struct LivestatusFixture
{
	LivestatusFixture()
	{
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
