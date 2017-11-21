/******************************************************************************
* Icinga 2                                                                   *
* Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#define VERSION "1.0.0"

#include "remote/httpclientconnection.hpp"
#include "remote/httprequest.hpp"
#include "remote/url-characters.hpp"
#include "base/application.hpp"
#include "base/json.hpp"
#include "base/string.hpp"
#include "base/exception.hpp"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

bool l_Debug = false;

/*
 * This function is called by an 'HttpRequest' once the server answers. After doing a short check on the 'response' it
 * decodes it to a Dictionary and then tells 'QueryEndpoint()' that it's done
 */
static void ResultHttpCompletionCallback(const HttpRequest& request, HttpResponse& response, bool& ready,
    boost::condition_variable& cv, boost::mutex& mtx, Dictionary::Ptr& result)
{
	String body;
	char buffer[1024];
	size_t count;

	while ((count = response.ReadBody(buffer, sizeof(buffer))) > 0)
		body += String(buffer, buffer + count);

	if (l_Debug) {
		std::cout << "Received answer\n"
		    << "\tHTTP code: " << response.StatusCode << "\n"
		    << "\tHTTP message: '" << response.StatusMessage << "'\n"
		    << "\tHTTP body: '" << body << "'.\n";
	}

	// Only try to decode the body if the 'HttpRequest' was successful
	if (response.StatusCode != 200)
		result = Dictionary::Ptr();
	else
		result = JsonDecode(body);

	// Unlock our mutex, set ready and notify 'QueryEndpoint()'
	boost::mutex::scoped_lock lock(mtx);
	ready = true;
	cv.notify_all();
}

/*
 * This function takes all the information required to query an nscp instance on
 * 'host':'port' with 'password'. The String 'endpoint' contains the specific
 * query name and all the arguments formatted as an URL.
 */
static Dictionary::Ptr QueryEndpoint(const String& host, const String& port, const String& password,
    const String& endpoint)
{
	HttpClientConnection::Ptr m_Connection = new HttpClientConnection(host, port, true);

	try {
		bool ready = false;
		boost::condition_variable cv;
		boost::mutex mtx;
		Dictionary::Ptr result;
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "GET";

		// Url() will call Utillity::UnescapeString() which will thrown an exception if it finds a lonely %
		req->RequestUrl = new Url(endpoint);
		req->AddHeader("password", password);
		if (l_Debug)
			std::cout << "Sending request to 'https://" << host << ":" << port << req->RequestUrl->Format() << "'\n";

		// Submits the request. The 'ResultHttpCompletionCallback' is called once the HttpRequest receives an answer,
		// which then sets 'ready' to true
		m_Connection->SubmitRequest(req, std::bind(ResultHttpCompletionCallback, _1, _2,
			boost::ref(ready), boost::ref(cv), boost::ref(mtx), boost::ref(result)));

		// We need to spinlock here because our 'HttpRequest' works asynchronous
		boost::mutex::scoped_lock lock(mtx);
		while (!ready) {
			cv.wait(lock);
		}

		return result;
	}
	catch (const std::exception& ex) {
		// Exceptions should only happen in extreme edge cases we can't recover from
		std::cout << "Caught exception: " << DiagnosticInformation(ex, false) << '\n';
		return Dictionary::Ptr();
	}
}

/*
 * Takes a Dictionary 'result' and constructs an icinga compliant output string.
 * If 'result' is not in the expected format it returns 3 ("UNKNOWN") and prints an informative, icinga compliant,
 * output string.
 */
static int FormatOutput(const Dictionary::Ptr& result)
{
	if (!result) {
		std::cout << "UNKNOWN: No data received.\n";
		return 3;
	}

	if (l_Debug)
		std::cout << "\tJSON Body:\n" << result->ToString() << '\n';

	Array::Ptr payloads = result->Get("payload");
	if (!payloads) {
		std::cout << "UNKNOWN: Answer format error: Answer is missing 'payload'.\n";
		return 3;
	}

	if (payloads->GetLength() == 0) {
		std::cout << "UNKNOWN: Answer format error: 'payload' was empty.\n";
		return 3;
	}

	if (payloads->GetLength() > 1) {
		std::cout << "UNKNOWN: Answer format error: Multiple payloads are not supported.";
		return 3;
	}

	Dictionary::Ptr payload;
	try {
		payload = payloads->Get(0);
	} catch (const std::exception& ex) {
		std::cout << "UNKNOWN: Answer format error: 'payload' was not a Dictionary.\n";
		return 3;
	}

	Array::Ptr lines;
	try {
		lines = payload->Get("lines");
	} catch (const std::exception&) {
		std::cout << "UNKNOWN: Answer format error: 'payload' is missing 'lines'.\n";
		return 3;
	}

	if (!lines) {
		std::cout << "UNKNOWN: Answer format error: 'lines' is Null.\n";
		return 3;
	}

	std::stringstream ssout;
	ObjectLock olock(lines);

	for (const Value& vline : lines) {
		Dictionary::Ptr line;
		try {
			line = vline;
		} catch (const std::exception& ex) {
			std::cout << "UNKNOWN: Answer format error: 'lines' entry was not a Dictionary.\n";
			return 3;
		}
		if (!line) {
			std::cout << "UNKNOWN: Answer format error: 'lines' entry was Null.\n";
			return 3;
		}

		ssout << payload->Get("command") << ' ' << line->Get("message") << " | ";

		if (!line->Contains("perf")) {
			ssout << '\n';
			break;
		}

		Array::Ptr perfs = line->Get("perf");
		ObjectLock olock(perfs);

		for (const Dictionary::Ptr& perf : perfs) {
			ssout << "'" << perf->Get("alias") << "'=";
			Dictionary::Ptr values = perf->Contains("int_value") ? perf->Get("int_value") : perf->Get("float_value");
			ssout << values->Get("value") << values->Get("unit") << ';' << values->Get("warning") << ';' << values->Get("critical");

			if (values->Contains("minimum") || values->Contains("maximum")) {
				ssout << ';';

				if (values->Contains("minimum"))
					ssout << values->Get("minimum");

				if (values->Contains("maximum"))
					ssout << ';' << values->Get("maximum");
			}

			ssout << ' ';
		}

		ssout << '\n';
	}

	//TODO: Fix
	String state = static_cast<String>(payload->Get("result")).ToUpper();
	int creturn = state == "OK" ? 0 :
		state == "WARNING" ? 1 :
		state == "CRITICAL" ? 2 :
		state == "UNKNOWN" ? 3 : 4;

	if (creturn == 4) {
		std::cout << "check_nscp UNKNOWN Answer format error: 'result' was not a known state.\n";
		return 3;
	}

	std::cout << ssout.rdbuf();
	return creturn;
}

/*
 *  Process arguments, initialize environment and shut down gracefully.
 */
int main(int argc, char **argv)
{
	po::variables_map vm;
	po::options_description desc("Options");

	desc.add_options()
		("help,h", "Print usage message and exit")
		("version,V", "Print version and exit")
		("debug,d", "Verbose/Debug output")
		("host,H", po::value<std::string>()->required(), "REQUIRED: NSCP API Host")
		("port,P", po::value<std::string>()->default_value("8443"), "NSCP API Port (Default: 8443)")
		("password", po::value<std::string>()->required(), "REQUIRED: NSCP API Password")
		("query,q", po::value<std::string>()->required(), "REQUIRED: NSCP API Query endpoint")
		("arguments,a", po::value<std::vector<std::string>>()->multitoken(), "NSCP API Query arguments for the endpoint");

	po::basic_command_line_parser<char> parser(argc, argv);

	try {
		po::store(
			parser
			.options(desc)
			.style(
				po::command_line_style::unix_style |
				po::command_line_style::allow_long_disguise)
			.run(),
			vm);

		if (vm.count("version")) {
			std::cout << "Version: " << VERSION << '\n';
			Application::Exit(0);
		}

		if (vm.count("help")) {
			std::cout << argv[0] << " Help\n\tVersion: " << VERSION << '\n';
			std::cout << "check_nscp_api is a program used to query the NSClient++ API.\n";
			std::cout << desc;
			std::cout << "For detailed information on possible queries and their arguments refer to the NSClient++ documentation.\n";
			Application::Exit(0);
		}

		vm.notify();
	} catch (std::exception& e) {
		std::cout << e.what() << '\n' << desc << '\n';
		Application::Exit(3);
	}

	if (vm.count("debug")) {
		l_Debug = true;
	}

	// Create the URL string and escape certain characters since Url() follows RFC 3986
	String endpoint = "/query/" + vm["query"].as<std::string>();
	if (!vm.count("arguments"))
		endpoint += '/';
	else {
		endpoint += '?';
		for (const String& argument : vm["arguments"].as<std::vector<std::string>>()) {
			String::SizeType pos = argument.FindFirstOf("=");
			if (pos == String::NPos)
				endpoint += Utility::EscapeString(argument, ACQUERY_ENCODE, false);
			else {
				String key = argument.SubStr(0, pos);
				String val = argument.SubStr(pos + 1);
				endpoint += Utility::EscapeString(key, ACQUERY_ENCODE, false) + "=" + Utility::EscapeString(val, ACQUERY_ENCODE, false);
			}
			endpoint += '&';
		}
	}

	// This needs to happen for HttpRequest to work
	Application::InitializeBase();

	Dictionary::Ptr result = QueryEndpoint(vm["host"].as<std::string>(), vm["port"].as<std::string>(),
	    vm["password"].as<std::string>(), endpoint);

	// Application::Exit() is the clean way to exit after calling InitializeBase()
	Application::Exit(FormatOutput(result));
	return 255;
}
