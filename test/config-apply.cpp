/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#include "config/applyrule.hpp"
#include "config/configcompiler.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

static Expression* RequireActualExpression(const std::unique_ptr<Expression>& compiledExpression)
{
	BOOST_REQUIRE_NE(compiledExpression.get(), nullptr);

	auto dict (dynamic_cast<DictExpression*>(compiledExpression.get()));
	BOOST_REQUIRE_NE(dict, nullptr);

	auto& subex (dict->GetExpressions());
	BOOST_REQUIRE_EQUAL(subex.size(), 1u);

	auto sub0 (subex.at(0).get());
	BOOST_REQUIRE_NE(sub0, nullptr);

	return sub0;
}

template<>
struct boost::test_tools::tt_detail::print_log_value<std::pair<String, String>>
{
	inline void operator()(std::ostream& os, const std::pair<String, String>& hs)
	{
		os << hs.first << "!" << hs.second;
	}
};

static void GetTargetHostsHelper(
	const String& filter, const Dictionary::Ptr& constants, bool targeted, const std::vector<String>& hosts = {}
)
{
	auto compiled (ConfigCompiler::CompileText("<test>", filter));
	auto expr (RequireActualExpression(compiled));
	std::vector<const String*> actualHosts;

	BOOST_CHECK_EQUAL(ApplyRule::GetTargetHosts(expr, actualHosts, constants), targeted);

	if (targeted) {
		std::vector<String> actualHostNames;

		actualHostNames.reserve(actualHosts.size());

		for (auto h : actualHosts) {
			actualHostNames.emplace_back(*h);
		}

		BOOST_CHECK_EQUAL_COLLECTIONS(actualHostNames.begin(), actualHostNames.end(), hosts.begin(), hosts.end());
	}
}

static void GetTargetServicesHelper(
	const String& filter, const Dictionary::Ptr& constants, bool targeted, const std::vector<std::pair<String, String>>& services = {}
)
{
	auto compiled (ConfigCompiler::CompileText("<test>", filter));
	auto expr (RequireActualExpression(compiled));
	std::vector<std::pair<const String*, const String*>> actualServices;

	BOOST_CHECK_EQUAL(ApplyRule::GetTargetServices(expr, actualServices, constants), targeted);

	if (targeted) {
		std::vector<std::pair<String, String>> actualServiceNames;

		actualServiceNames.reserve(actualServices.size());

		for (auto s : actualServices) {
			actualServiceNames.emplace_back(*s.first, *s.second);
		}

		BOOST_CHECK_EQUAL_COLLECTIONS(actualServiceNames.begin(), actualServiceNames.end(), services.begin(), services.end());
	}
}

BOOST_AUTO_TEST_SUITE(config_apply)

BOOST_AUTO_TEST_CASE(gettargethosts_literal)
{
	GetTargetHostsHelper("host.name == \"foo\"", nullptr, true, {"foo"});
}

BOOST_AUTO_TEST_CASE(gettargethosts_const)
{
	GetTargetHostsHelper("host.name == x", new Dictionary({{"x", "foo"}}), true, {"foo"});
}

BOOST_AUTO_TEST_CASE(gettargethosts_swapped)
{
	GetTargetHostsHelper("\"foo\" == host.name", nullptr, true, {"foo"});
}

BOOST_AUTO_TEST_CASE(gettargethosts_two)
{
	GetTargetHostsHelper("host.name == \"foo\" || host.name == \"bar\"", nullptr, true, {"foo", "bar"});
}

BOOST_AUTO_TEST_CASE(gettargethosts_three)
{
	GetTargetHostsHelper(
		"host.name == \"foo\" || host.name == \"bar\" || host.name == \"foobar\"",
		nullptr, true, {"foo", "bar", "foobar"}
	);
}

BOOST_AUTO_TEST_CASE(gettargethosts_mixed)
{
	GetTargetHostsHelper("host.name == x || \"bar\" == host.name", new Dictionary({{"x", "foo"}}), true, {"foo", "bar"});
}

BOOST_AUTO_TEST_CASE(gettargethosts_redundant)
{
	GetTargetHostsHelper("host.name == \"foo\" && 1", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargethosts_badconst)
{
	GetTargetHostsHelper("host.name == NodeName", new Dictionary({{"x", "foo"}}), false);
}

BOOST_AUTO_TEST_CASE(gettargethosts_notliteral)
{
	GetTargetHostsHelper("host.name == \"foo\" + \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargethosts_wrongop)
{
	GetTargetHostsHelper("host.name != \"foo\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargethosts_wrongattr)
{
	GetTargetHostsHelper("host.__name == \"foo\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargethosts_wrongvar)
{
	GetTargetHostsHelper("service.name == \"foo\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargethosts_noindexer)
{
	GetTargetHostsHelper("name == \"foo\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_literal)
{
	GetTargetServicesHelper("host.name == \"foo\" && service.name == \"bar\"", nullptr, true, {{"foo", "bar"}});
}

BOOST_AUTO_TEST_CASE(gettargetservices_const)
{
	GetTargetServicesHelper("host.name == x && service.name == y", new Dictionary({{"x", "foo"}, {"y", "bar"}}), true, {{"foo", "bar"}});
}

BOOST_AUTO_TEST_CASE(gettargetservices_swapped_outer)
{
	GetTargetServicesHelper("service.name == \"bar\" && host.name == \"foo\"", nullptr, true, {{"foo", "bar"}});
}

BOOST_AUTO_TEST_CASE(gettargetservices_swapped_inner)
{
	GetTargetServicesHelper("\"foo\" == host.name && \"bar\" == service.name", nullptr, true, {{"foo", "bar"}});
}

BOOST_AUTO_TEST_CASE(gettargetservices_two)
{
	GetTargetServicesHelper(
		"host.name == \"foo\" && service.name == \"bar\" || host.name == \"oof\" && service.name == \"rab\"",
		nullptr, true, {{"foo", "bar"}, {"oof", "rab"}}
	);
}

BOOST_AUTO_TEST_CASE(gettargetservices_three)
{
	GetTargetServicesHelper(
		"host.name == \"foo\" && service.name == \"bar\" || host.name == \"oof\" && service.name == \"rab\" || host.name == \"ofo\" && service.name == \"rba\"",
		nullptr, true, {{"foo", "bar"}, {"oof", "rab"}, {"ofo", "rba"}}
	);
}

BOOST_AUTO_TEST_CASE(gettargetservices_mixed)
{
	GetTargetServicesHelper("\"bar\" == service.name && x == host.name", new Dictionary({{"x", "foo"}}), true, {{"foo", "bar"}});
}

BOOST_AUTO_TEST_CASE(gettargetservices_redundant)
{
	GetTargetServicesHelper("host.name == \"foo\" && service.name == \"bar\" && 1", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_badconst)
{
	GetTargetServicesHelper("host.name == NodeName && service.name == \"bar\"", new Dictionary({{"x", "foo"}}), false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_notliteral)
{
	GetTargetServicesHelper("host.name == \"foo\" && service.name == \"b\" + \"ar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongop_outer)
{
	GetTargetServicesHelper("host.name == \"foo\" & service.name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongop_host)
{
	GetTargetServicesHelper("host.name != \"foo\" && service.name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongop_service)
{
	GetTargetServicesHelper("host.name == \"foo\" && service.name != \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongattr_host)
{
	GetTargetServicesHelper("host.__name == \"foo\" && service.name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongattr_service)
{
	GetTargetServicesHelper("host.name == \"foo\" && service.__name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongvar_host)
{
	GetTargetServicesHelper("horst.name == \"foo\" && service.name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_wrongvar_service)
{
	GetTargetServicesHelper("host.name == \"foo\" && sehrvice.name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_noindexer_host)
{
	GetTargetServicesHelper("name == \"foo\" && service.name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_CASE(gettargetservices_noindexer_service)
{
	GetTargetServicesHelper("host.name == \"foo\" && name == \"bar\"", nullptr, false);
}

BOOST_AUTO_TEST_SUITE_END()
