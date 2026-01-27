// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/array.hpp"
#include "remote/url.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(remote_url)

BOOST_AUTO_TEST_CASE(id_and_path)
{
	Url::Ptr url = new Url("http://icinga.com/foo/bar/baz?hurr=durr");

	BOOST_CHECK(url->GetScheme() == "http");

	BOOST_CHECK(url->GetAuthority() == "icinga.com");

	std::vector<String> PathCorrect;
	PathCorrect.emplace_back("foo");
	PathCorrect.emplace_back("bar");
	PathCorrect.emplace_back("baz");

	BOOST_CHECK(url->GetPath() == PathCorrect);
}

BOOST_AUTO_TEST_CASE(get_and_set)
{
	Url::Ptr url = new Url();
	url->SetScheme("ftp");
	url->SetUsername("Horst");
	url->SetPassword("Seehofer");
	url->SetHost("koenigreich.bayern");
	url->SetPort("1918");
	url->SetPath({ "path", "to", "münchen" });

	BOOST_CHECK(url->Format(false, true) == "ftp://Horst:Seehofer@koenigreich.bayern:1918/path/to/m%C3%BCnchen");

	url->SetQuery({
		{"shout", "hip"},
		{"shout", "hip"},
		{"shout", "hurra"},
		{"sonderzeichen", "äü^ä+#ül-"}
	});
	url->AddQueryElement("count", "3");

	auto mn (url->GetQuery());

	BOOST_CHECK(mn.size() == 5);

	BOOST_CHECK(mn[0].first == "shout");
	BOOST_CHECK(mn[0].second == "hip");

	BOOST_CHECK(mn[1].first == "shout");
	BOOST_CHECK(mn[1].second == "hip");

	BOOST_CHECK(mn[2].first == "shout");
	BOOST_CHECK(mn[2].second == "hurra");

	BOOST_CHECK(mn[3].first == "sonderzeichen");
	BOOST_CHECK(mn[3].second == "äü^ä+#ül-");

	BOOST_CHECK(mn[4].first == "count");
	BOOST_CHECK(mn[4].second == "3");
}

BOOST_AUTO_TEST_CASE(parameters)
{
	Url::Ptr url = new Url("https://icinga.com/hya/?rair=robert&rain=karl&foo[]=bar");

	auto query (url->GetQuery());

	BOOST_CHECK(query.size() == 3);

	BOOST_CHECK(query[0].first == "rair");
	BOOST_CHECK(query[0].second == "robert");

	BOOST_CHECK(query[1].first == "rain");
	BOOST_CHECK(query[1].second == "karl");

	BOOST_CHECK(query[2].first == "foo");
	BOOST_CHECK(query[2].second == "bar");
}

BOOST_AUTO_TEST_CASE(format)
{
	Url::Ptr url = new Url("http://foo.bar/baz/?hop=top&flop=sop#iLIKEtrains");
	Url::Ptr url2;
	BOOST_CHECK(url2 = new Url(url->Format(false, false)));

	url = new Url("//main.args/////////?k[]=one&k[]=two#three");
	BOOST_CHECK(url2 = new Url(url->Format(false, false)));

	url = new Url("/foo/bar/index.php?blaka");
	BOOST_CHECK(url2 = new Url(url->Format(false, false)));
	BOOST_CHECK(url->Format(false, false) == "/foo/bar/index.php?blaka");

	url = new Url("/");
	BOOST_CHECK(url->Format(false, false) == "/");

	url = new Url("https://nsclient:8443/query/check_cpu?time%5B%5D=1m&time=5m&time%5B%5D=15m");
	url->SetArrayFormatUseBrackets(false);
	BOOST_CHECK(url2 = new Url(url->Format(false, false)));

	url = new Url("https://icinga2/query?a[]=1&a[]=2&a[]=3");
	url->SetArrayFormatUseBrackets(true);
	BOOST_CHECK(url2 = new Url(url->Format(false, false)));
}

BOOST_AUTO_TEST_CASE(illegal_legal_strings)
{
	Url::Ptr url;
	BOOST_CHECK(url = new Url("/?foo=barr&foo[]=bazz"));
	BOOST_CHECK_THROW(url = new Url("/?]=gar"), std::invalid_argument);
	BOOST_CHECK_THROW(url = new Url("/#?[]"), std::invalid_argument);
	BOOST_CHECK(url = new Url("/?foo=bar&foo=ba"));
	BOOST_CHECK_THROW(url = new Url("/?foo=bar&[]=d"), std::invalid_argument);
	BOOST_CHECK(url = new Url("/?fo=&bar=garOA"));
	BOOST_CHECK(url = new Url("https://127.0.0.1:5665/demo?type=Service&filter=service.state%3E0"));
	BOOST_CHECK(url = new Url("/?foo=baz??&\?\?=/?"));
	BOOST_CHECK(url = new Url("/"));
	BOOST_CHECK(url = new Url("///////"));
	BOOST_CHECK(url = new Url("/??[]=?#?=?"));
	BOOST_CHECK(url = new Url("http://foo/#bar"));
	BOOST_CHECK(url = new Url("//foo/"));
}

BOOST_AUTO_TEST_SUITE_END()
