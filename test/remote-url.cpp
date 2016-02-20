/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "base/array.hpp"
#include "remote/url.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(remote_url)

BOOST_AUTO_TEST_CASE(id_and_path)
{
	Url::Ptr url = new Url("http://icinga.org/foo/bar/baz?hurr=durr");

	BOOST_CHECK(url->GetScheme() == "http");

	BOOST_CHECK(url->GetAuthority() == "icinga.org");

	std::vector<String> PathCorrect;
	PathCorrect.push_back("foo");
	PathCorrect.push_back("bar");
	PathCorrect.push_back("baz");

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
	std::vector<String> p = boost::assign::list_of("path")("to")("münchen");
	url->SetPath(p);
	BOOST_CHECK(url->Format(true) == "ftp://Horst:Seehofer@koenigreich.bayern:1918/path/to/m%C3%BCnchen");

	std::map<String, std::vector<String> > m;
	std::vector<String> v1 = boost::assign::list_of("hip")("hip")("hurra");
	std::vector<String> v2 = boost::assign::list_of("äü^ä+#ül-");
	std::vector<String> v3 = boost::assign::list_of("1")("2");
	m.insert(std::pair<String, std::vector<String> >("shout", v1));
	m.insert(std::pair<String, std::vector<String> >("sonderzeichen", v2));
	url->SetQuery(m);
	url->SetQueryElements("count", v3);
	url->AddQueryElement("count", "3");

	std::map<String, std::vector<String> > mn = url->GetQuery();
	BOOST_CHECK(mn["shout"][0] == v1[0]);
	BOOST_CHECK(mn["sonderzeichen"][0] == v2[0]);
	BOOST_CHECK(mn["count"][2] == "3");
}

BOOST_AUTO_TEST_CASE(parameters)
{
	Url::Ptr url = new Url("https://icinga.org/hya/?rain=karl&rair=robert&foo[]=bar");

	BOOST_CHECK(url->GetQueryElement("rair") == "robert");
	BOOST_CHECK(url->GetQueryElement("rain") == "karl");
	std::vector<String> test = url->GetQueryElements("foo");
	BOOST_CHECK(test.size() == 1);
	BOOST_CHECK(test[0] == "bar");
}

BOOST_AUTO_TEST_CASE(format)
{
	Url::Ptr url = new Url("http://foo.bar/baz/?hop=top&flop=sop#iLIKEtrains");
	BOOST_CHECK(new Url(url->Format()));

	url = new Url("//main.args/////////?k[]=one&k[]=two#three");
	BOOST_CHECK(new Url(url->Format()));

	url = new Url("/foo/bar/index.php?blaka");
	BOOST_CHECK(new Url(url->Format()));

	url = new Url("/");
	BOOST_CHECK(url->Format() == "/");
}

BOOST_AUTO_TEST_CASE(illegal_legal_strings)
{
	BOOST_CHECK(new Url("/?foo=barr&foo[]=bazz"));
	BOOST_CHECK_THROW(new Url("/?]=gar"), std::invalid_argument);
	BOOST_CHECK_THROW(new Url("/#?[]"), std::invalid_argument);
	BOOST_CHECK(new Url("/?foo=bar&foo=ba"));
	BOOST_CHECK_THROW(new Url("/?foo=bar&[]=d"), std::invalid_argument);
	BOOST_CHECK(new Url("/?fo=&bar=garOA"));
	BOOST_CHECK(new Url("https://127.0.0.1:5665/demo?type=Service&filter=service.state%3E0"));
	BOOST_CHECK(new Url("/?foo=baz??&\?\?=/?"));
	BOOST_CHECK(new Url("/"));
	BOOST_CHECK(new Url("///////"));
	BOOST_CHECK(new Url("/??[]=?#?=?"));
	BOOST_CHECK(new Url("http://foo/#bar"));
	BOOST_CHECK(new Url("//foo/"));
}

BOOST_AUTO_TEST_SUITE_END()
