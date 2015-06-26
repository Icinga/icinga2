/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/url.hpp"
#include "base/array.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_url)

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

BOOST_AUTO_TEST_CASE(parameters)
{
	Url::Ptr url = new Url("https://icinga.org/hya/?rain=karl&rair=robert&foo[]=bar");

	BOOST_CHECK(url->GetQueryElement("rair") == "robert");
	BOOST_CHECK(url->GetQueryElement("rain") == "karl");
	BOOST_CHECK(url->GetQueryElement("foo").IsObjectType<Array>());
	Array::Ptr test = url->GetQueryElement("foo");
	BOOST_CHECK(test->GetLength() == 1);
	BOOST_CHECK(test->Get(0) == "bar");
}

BOOST_AUTO_TEST_CASE(format)
{
	Url::Ptr url = new Url("http://foo.bar/baz/?hop=top&flop=sop#iLIKEtrains");
	BOOST_CHECK(new Url(url->Format()));

	url = new Url("//main.args/////////?k[]=one&k[]=two#three");
	BOOST_CHECK(new Url(url->Format()));

	url = new Url("/foo/bar/index.php?blaka");
	BOOST_CHECK(new Url(url->Format()));
}

BOOST_AUTO_TEST_CASE(illegal_legal_strings)
{
	BOOST_CHECK_THROW(new Url("/?foo=barr&foo[]=bazz"), std::invalid_argument);
	BOOST_CHECK_THROW(new Url("/?]=gar"), std::invalid_argument);
	BOOST_CHECK(new Url("/?foo=baz??&\?\?=/?")); //Valid
	BOOST_CHECK_THROW(new Url("/?foo=bar&foo=ba"), std::invalid_argument);
	BOOST_CHECK_THROW(new Url("/?foo=bar&[]=d"), std::invalid_argument);
	BOOST_CHECK_THROW(new Url("/?fo=&bar=garOA"), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
