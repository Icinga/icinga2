/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "icinga/host.hpp"
#include "remote/apiuser.hpp"
#include "remote/filterutility.hpp"
#include "test/icingaapplication-fixture.hpp"
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"

using namespace icinga;

// clang-format off
BOOST_AUTO_TEST_SUITE(remote_filterutility,
	*boost::unit_test::label("config"))
// clang-format on

BOOST_FIXTURE_TEST_CASE(safe_function_permissions, IcingaApplicationFixture)
{
	auto createObjects = []() {
		String config = R"CONFIG({
object CheckCommand "dummy" {
  command = "/bin/echo"
  execute = {{ throw "not implemented" }}
}

object ApiUser "allPermissionsUser" {
  permissions = [ "*" ]
}

object ApiUser "permissionFilterUser" {
  permissions = [
    {
      permission = "objects/query/Host"
      filter = {{ host.name == {{{host1}}} }}
    },
    {
      permission = "objects/query/Service"
      filter = {{ service.name == {{{svc1}}} }}
    }
  ]
}

object Host "host1" {
  address = "host1"
  check_command = "dummy"
}

object Host "host2" {
  address = "host2"
  check_command = "dummy"
}

object Service "svc1" {
  host_name = "host1"
  check_command = "dummy"
}

object Service "svc2" {
  host_name = "host2"
  check_command = "dummy"
}
})CONFIG";
		std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
		expr->Evaluate(*ScriptFrame::GetCurrentFrame());
	};

	ConfigItem::RunWithActivationContext(new Function("CreateTestObjects", createObjects));

	auto allPermissionsUser = ApiUser::GetByName("allPermissionsUser");
	auto permissionFilterUser = ApiUser::GetByName("permissionFilterUser");

	QueryDescription qd;
	qd.Types.insert("Host");
	qd.Types.insert("Service");
	qd.Permission = "objects/query/Host";

	Dictionary::Ptr queryParams = new Dictionary();
	queryParams->Set("type", "Host");

	// This is a filter that uses a get_object call on an object the permissionFilterUser
	// has access to. A second user is tested that has access to everything, to make sure
	// the filter evaluates properly in the first place.
	queryParams->Set("filter", "get_object(Host,{{{host1}}}).name == {{{host1}}}");

	std::vector<Value> objs;
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 2);

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);

	// We need to test again with querying services, while still using the get_object(Host) filter,
	// because we need to verify permissions in filters work regardless of whether the object type
	// that is queried is the same or different from the one that is checked in the filters.
	qd.Permission = "objects/query/Service";
	queryParams->Set("type", "Service");

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 2);

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);

	// Now test again with a filter that always evaluates to false.
	// Both users shouldn't find objects.
	queryParams->Set("filter", "get_object(Host,{{{host2}}}).name == {{{host1}}}");
	qd.Permission = "objects/query/Host";
	queryParams->Set("type", "Host");

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK(objs.empty());

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK(objs.empty());

	// Again, the same test with querying service objects instead of hosts.
	qd.Permission = "objects/query/Service";
	queryParams->Set("type", "Service");

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK(objs.empty());

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK(objs.empty());

	// In the previous asserts we have established that filters work as intended with valid permissions.
	// Now test again with a valid filter that tries to access a host object the permissionFilterUser
	// doesn't have access to. It should still return an empty array.
	queryParams->Set("filter", "get_object(Host,{{{host2}}}).name == {{{host2}}}");
	qd.Permission = "objects/query/Host";
	queryParams->Set("type", "Host");

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 2);

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK(objs.empty());

	// Again, the same test with querying service objects instead of hosts.
	qd.Permission = "objects/query/Service";
	queryParams->Set("type", "Service");

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 2);

	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK(objs.empty());
}

BOOST_FIXTURE_TEST_CASE(variable_expression_permissions, IcingaApplicationFixture)
{
	auto createObjects = []() {
		String config = R"CONFIG({
object CheckCommand "dummy" {
  command = "/bin/echo"
  execute = {{ throw "not implemented" }}
}

object ApiUser "allPermissionsUser" {
  permissions = [ "*" ]
}

object ApiUser "permissionFilterUser" {
  permissions = [
    "objects/query/Host",
    {
      permission = "variables"
      filter = {{ variable.name != {{{SuperSecretConstant}}} }}
    }
  ]
}

object ApiUser "noVariablePermUser" {
  permissions = [ "objects/query/Host" ]
}

object Host "host1" {
  address = "host1"
  check_command = "dummy"
}
})CONFIG";
		std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
		expr->Evaluate(*ScriptFrame::GetCurrentFrame());
	};

	ConfigItem::RunWithActivationContext(new Function("CreateTestObjects", createObjects));

	auto allPermissionsUser = ApiUser::GetByName("allPermissionsUser");
	auto permissionFilterUser = ApiUser::GetByName("permissionFilterUser");
	auto noVariablePermUser = ApiUser::GetByName("noVariablePermUser");

	QueryDescription qd;
	qd.Types.insert("Host");
	qd.Types.insert("Service");
	qd.Permission = "objects/query/Host";

	ScriptGlobal::Set("VeryUsefulConstant", "Test1");
	ScriptGlobal::Set("SuperSecretConstant", "MyCleartextBankingPassword");
	ScriptGlobal::Set("TicketSalt", "Test2");

	Dictionary::Ptr queryParams = new Dictionary();
	queryParams->Set("type", "Host");

	std::vector<Value> objs;

	// First test a simple variable access.
	// We expect the user with the right permissions to be able to query the object,
	// while for a user without permission a ScriptError should be thrown.
	queryParams->Set("filter", "VeryUsefulConstant == {{{Test1}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser), ScriptError);

	// The variable can also be referenced and dereferenced.
	// Unlike the variable access above indirectly accessing the variable without permissions does not
	// throw a ScriptError but just returns an empty string.
	queryParams->Set("filter", "*&VeryUsefulConstant == {{{Test1}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// The variable can also be referenced and dereferenced via get(), which should have the
	// same result as above.
	queryParams->Set("filter", "(&VeryUsefulConstant).get() == {{{Test1}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// Global variables can also be accessed via an IndexerExpression. The result should be the same as above.
	queryParams->Set("filter", "globals[{{{VeryUsefulConstant}}}] == {{{Test1}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// Now we verify that a user that isn't allowed to access a constant is not able to do so in a
	// filter expression.
	// The allPermissionsUser should be able to access the variable.
	// The permissionFilterUser should receive an exception because they are specifically
	// forbidden from reading that variable.
	// Same for the noVariablePermUser, which as before isn't allowed to read any variable.
	queryParams->Set("filter", "SuperSecretConstant == {{{MyCleartextBankingPassword}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser), ScriptError);
	BOOST_REQUIRE_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser), ScriptError);

	// Repeat the other ways to access secret variables, again, only the allPermissionsUser should
	// be able to use it.
	queryParams->Set("filter", "*&SuperSecretConstant == {{{MyCleartextBankingPassword}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// Repeat the other ways to access secret variables, again, only the allPermissionsUser should
	// be able to use it.
	queryParams->Set("filter", "(&SuperSecretConstant).get() == {{{MyCleartextBankingPassword}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// Repeat the other ways to access secret variables, again, only the allPermissionsUser should
	// be able to use it.
	queryParams->Set("filter", "globals[{{{SuperSecretConstant}}}] == {{{MyCleartextBankingPassword}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 1);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// We also need to verify that even a user with all permissions can not access the TicketSalt variable.
	// Like in the other cases above, direct access should throw.
	queryParams->Set("filter", "TicketSalt == {{{Test2}}}");
	BOOST_REQUIRE_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser), ScriptError);
	BOOST_REQUIRE_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser), ScriptError);
	BOOST_REQUIRE_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser), ScriptError);

	// Repeat the other ways to access variables with TicketSalt.
	queryParams->Set("filter", "*&TicketSalt == {{{Test2}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// Repeat the other ways to access variables with TicketSalt.
	queryParams->Set("filter", "(&TicketSalt).get() == {{{Test2}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);

	// Repeat the other ways to access variables with TicketSalt.
	queryParams->Set("filter", "globals[{{{TicketSalt}}}] == {{{Test2}}}");
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, allPermissionsUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, permissionFilterUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
	BOOST_REQUIRE_NO_THROW(objs = FilterUtility::GetFilterTargets(qd, queryParams, noVariablePermUser));
	BOOST_CHECK_EQUAL(objs.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
