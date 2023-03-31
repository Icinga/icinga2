/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga-version.h" /* include VERSION */

// ensure to include base first
#include "base/i2-base.hpp"
#include "base/application.hpp"
#include "base/json.hpp"
#include "base/string.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/stream.hpp"
#include "base/tcpsocket.hpp" /* include global icinga::Connect */
#include "base/tlsstream.hpp"
#include "base/base64.hpp"
#include "remote/url.hpp"
#include <remote/url-characters.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

static bool l_Debug;

/**
 * Prints an Icinga plugin API compliant output, including error handling.
 *
 * @param result
 *
 * @return Status code for exit()
 */
static int FormatOutput(const Dictionary::Ptr& result)
{
	if (!result) {
		std::cerr << "UNKNOWN: No data received.\n";
		return 3;
	}

	if (l_Debug)
		std::cout << "\tJSON Body:\n" << result->ToString() << '\n';

	Array::Ptr payloads = result->Get("payload");
	if (!payloads) {
		std::cerr << "UNKNOWN: Answer format error: Answer is missing 'payload'.\n";
		return 3;
	}

	if (payloads->GetLength() == 0) {
		std::cerr << "UNKNOWN: Answer format error: 'payload' was empty.\n";
		return 3;
	}

	if (payloads->GetLength() > 1) {
		std::cerr << "UNKNOWN: Answer format error: Multiple payloads are not supported.";
		return 3;
	}

	Dictionary::Ptr payload;

	try {
		payload = payloads->Get(0);
	} catch (const std::exception&) {
		std::cerr << "UNKNOWN: Answer format error: 'payload' was not a Dictionary.\n";
		return 3;
	}

	Array::Ptr lines;

	try {
		lines = payload->Get("lines");
	} catch (const std::exception&) {
		std::cerr << "UNKNOWN: Answer format error: 'payload' is missing 'lines'.\n";
		return 3;
	}

	if (!lines) {
		std::cerr << "UNKNOWN: Answer format error: 'lines' is Null.\n";
		return 3;
	}

	std::stringstream ssout;

	ObjectLock olock(lines);

	for (const Value& vline : lines) {
		Dictionary::Ptr line;

		try {
			line = vline;
		} catch (const std::exception&) {
			std::cerr << "UNKNOWN: Answer format error: 'lines' entry was not a Dictionary.\n";
			return 3;
		}

		if (!line) {
			std::cerr << "UNKNOWN: Answer format error: 'lines' entry was Null.\n";
			return 3;
		}

		ssout << payload->Get("command") << ' ' << line->Get("message") << " | ";

		if (!line->Contains("perf")) {
			ssout << '\n';
			break;
		}

		Array::Ptr perfs = line->Get("perf");

		ObjectLock olock(perfs);

		for (Dictionary::Ptr perf : perfs) {
			ssout << "'" << perf->Get("alias") << "'=";

			Dictionary::Ptr values = perf->Get("float_value");

			if (perf->Contains("int_value"))
				values = perf->Get("int_value");

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

	std::map<String, unsigned int> stateMap = {
		{ "OK", 0 },
		{ "WARNING", 1},
		{ "CRITICAL", 2},
		{ "UNKNOWN", 3}
	};

	String state = static_cast<String>(payload->Get("result")).ToUpper();

	auto it = stateMap.find(state);

	if (it == stateMap.end()) {
		std::cerr << "UNKNOWN Answer format error: 'result' was not a known state.\n";
		return 3;
	}

	std::cout << ssout.rdbuf();

	return it->second;
}

/**
 * Connects to host:port and performs a TLS shandshake
 *
 * @param host To connect to.
 * @param port To connect to.
 *
 * @returns AsioTlsStream pointer for future HTTP connections.
 */
static Shared<AsioTlsStream>::Ptr Connect(const String& host, const String& port)
{
	Shared<boost::asio::ssl::context>::Ptr sslContext;

	try {
		sslContext = MakeAsioSslContext(Empty, Empty, Empty); //TODO: Add support for cert, key, ca parameters
	} catch(const std::exception& ex) {
		Log(LogCritical, "DebugConsole")
			<< "Cannot make SSL context: " << ex.what();
		throw;
	}

	Shared<AsioTlsStream>::Ptr stream = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslContext, host);

	try {
		icinga::Connect(stream->lowest_layer(), host, port);
	} catch (const std::exception& ex) {
		Log(LogWarning, "DebugConsole")
			<< "Cannot connect to REST API on host '" << host << "' port '" << port << "': " << ex.what();
		throw;
	}

	auto& tlsStream (stream->next_layer());

	try {
		tlsStream.handshake(tlsStream.client);
	} catch (const std::exception& ex) {
		Log(LogWarning, "DebugConsole")
			<< "TLS handshake with host '" << host << "' failed: " << ex.what();
		throw;
	}

	return stream;
}

static const char l_ReasonToInject[2] = {' ', 'X'};

template<class MutableBufferSequence>
static inline
boost::asio::mutable_buffer GetFirstNonZeroBuffer(const MutableBufferSequence& mbs)
{
	namespace asio = boost::asio;

	auto end (asio::buffer_sequence_end(mbs));

	for (auto current (asio::buffer_sequence_begin(mbs)); current != end; ++current) {
		asio::mutable_buffer buf (*current);

		if (buf.size() > 0u) {
			return buf;
		}
	}

	return {};
}

/**
 * Workaround for <https://github.com/mickem/nscp/issues/610>.
 */
template<class SyncReadStream>
class HttpResponseReasonInjector
{
public:
	inline HttpResponseReasonInjector(SyncReadStream& stream)
		: m_Stream(stream), m_ReasonHasBeenInjected(false), m_StashedData(nullptr)
	{
	}

	HttpResponseReasonInjector(const HttpResponseReasonInjector&) = delete;
	HttpResponseReasonInjector(HttpResponseReasonInjector&&) = delete;
	HttpResponseReasonInjector& operator=(const HttpResponseReasonInjector&) = delete;
	HttpResponseReasonInjector& operator=(HttpResponseReasonInjector&&) = delete;

	template<class MutableBufferSequence>
	size_t read_some(const MutableBufferSequence& mbs)
	{
		boost::system::error_code ec;
		size_t amount = read_some(mbs, ec);

		if (ec) {
			throw boost::system::system_error(ec);
		}

		return amount;
	}

	template<class MutableBufferSequence>
	size_t read_some(const MutableBufferSequence& mbs, boost::system::error_code& ec)
	{
		auto mb (GetFirstNonZeroBuffer(mbs));

		if (m_StashedData) {
			size_t amount = 0;
			auto end ((char*)mb.data() + mb.size());

			for (auto current ((char*)mb.data()); current < end; ++current) {
				*current = *m_StashedData;

				++m_StashedData;
				++amount;

				if (m_StashedData == (char*)m_StashedDataBuf + (sizeof(m_StashedDataBuf) / sizeof(m_StashedDataBuf[0]))) {
					m_StashedData = nullptr;
					break;
				}
			}

			return amount;
		}

		size_t amount = m_Stream.read_some(mb, ec);

		if (!ec && !m_ReasonHasBeenInjected) {
			auto end ((char*)mb.data() + amount);

			for (auto current ((char*)mb.data()); current < end; ++current) {
				if (*current == '\r') {
					auto last (end - 1);

					for (size_t i = sizeof(l_ReasonToInject) / sizeof(l_ReasonToInject[0]); i;) {
						m_StashedDataBuf[--i] = *last;

						if (last > current) {
							memmove(current + 1, current, last - current);
						}

						*current = l_ReasonToInject[i];
					}

					m_ReasonHasBeenInjected = true;
					m_StashedData = m_StashedDataBuf;

					break;
				}
			}
		}

		return amount;
	}

private:
	SyncReadStream& m_Stream;
	bool m_ReasonHasBeenInjected;
	char m_StashedDataBuf[sizeof(l_ReasonToInject) / sizeof(l_ReasonToInject[0])];
	char* m_StashedData;
};

/**
 * Queries the given endpoint and host:port and retrieves data.
 *
 * @param host To connect to.
 * @param port To connect to.
 * @param password For auth header (required).
 * @param endpoint Caller must construct the full endpoint including the command query.
 *
 * @return Dictionary de-serialized from JSON data.
 */

static Dictionary::Ptr FetchData(const String& host, const String& port, const String& password,
	const String& endpoint)
{
	namespace beast = boost::beast;
	namespace http = beast::http;

	Shared<AsioTlsStream>::Ptr tlsStream;

	try {
		tlsStream = Connect(host, port);
	} catch (const std::exception& ex) {
		std::cerr << "Connection error: " << ex.what();
		throw ex;
	}

	Url::Ptr url;

	try {
		url = new Url(endpoint);
	} catch (const std::exception& ex) {
		std::cerr << "URL error: " << ex.what();
		throw ex;
	}

	url->SetScheme("https");
	url->SetHost(host);
	url->SetPort(port);

	// NSClient++ uses `time=1m&time=5m` instead of `time[]=1m&time[]=5m`
	url->SetArrayFormatUseBrackets(false);

	http::request<http::string_body> request (http::verb::get, std::string(url->Format(true)), 10);

	request.set(http::field::user_agent, "Icinga/check_nscp_api/" + String(VERSION));
	request.set(http::field::host, host + ":" + port);

	request.set(http::field::accept, "application/json");
	request.set("password", password);

	if (l_Debug) {
		std::cout << "Sending request to " << url->Format(false, false) << "'.\n";
	}

	try {
		http::write(*tlsStream, request);
		tlsStream->flush();
	} catch (const std::exception& ex) {
		std::cerr << "Cannot write HTTP request to REST API at URL '" << url->Format(false, false) << "': " << ex.what();
		throw ex;
	}

	beast::flat_buffer buffer;
	http::parser<false, http::string_body> p;

	try {
		HttpResponseReasonInjector<decltype(*tlsStream)> reasonInjector (*tlsStream);
		http::read(reasonInjector, buffer, p);
	} catch (const std::exception &ex) {
		BOOST_THROW_EXCEPTION(ScriptError(String("Error reading HTTP response data: ") + ex.what()));
	}

	String body (std::move(p.get().body()));

	if (l_Debug)
		std::cout << "Received body from NSCP: '" << body << "'." << std::endl;

	// Add some rudimentary error handling.
	if (body.IsEmpty()) {
		String message = "No body received. Ensure that connection parameters are good and check the NSCP logs.";
		BOOST_THROW_EXCEPTION(ScriptError(message));
	}

	Dictionary::Ptr jsonResponse;

	try {
		jsonResponse = JsonDecode(body);
	} catch (const std::exception& ex) {
		String message = "Cannot parse JSON response body '" + body + "', error: " + ex.what();
		BOOST_THROW_EXCEPTION(ScriptError(message));
	}

	return jsonResponse;
}

/**
 * Main function
 *
 * @param argc
 * @param argv
 * @return exit code
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

	po::command_line_parser parser(argc, argv);

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
	} catch (const std::exception& e) {
		std::cout << e.what() << '\n' << desc << '\n';
		Application::Exit(3);
	}

	l_Debug = vm.count("debug") > 0;

	// Initialize logger
	if (l_Debug)
		Logger::SetConsoleLogSeverity(LogDebug);
	else
		Logger::SetConsoleLogSeverity(LogWarning);

	// Create the URL string and escape certain characters since Url() follows RFC 3986
	String endpoint = "/query/" + vm["query"].as<std::string>();
	if (!vm.count("arguments"))
		endpoint += '/';
	else {
		endpoint += '?';
		for (String argument : vm["arguments"].as<std::vector<std::string>>()) {
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

	Dictionary::Ptr result;

	try {
		result = FetchData(vm["host"].as<std::string>(), vm["port"].as<std::string>(),
		   vm["password"].as<std::string>(), endpoint);
	} catch (const std::exception& ex) {
		std::cerr << "UNKNOWN - " << ex.what();
		exit(3);
	}

	// Application::Exit() is the clean way to exit after calling InitializeBase()
	Application::Exit(FormatOutput(result));
	return 255;
}
