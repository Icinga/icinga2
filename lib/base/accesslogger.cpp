/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/accesslogger.hpp"
#include "base/accesslogger-ti.cpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include "base/application.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <cstring>
#include <unordered_map>
#include <stdexcept>
#include <time.h>
#include <utility>
#include <vector>

using namespace icinga;
namespace http = boost::beast::http;

static std::set<AccessLogger::Ptr> l_AccessLoggers;
static boost::mutex l_AccessLoggersMutex;

REGISTER_TYPE(AccessLogger);

REGISTER_STATSFUNCTION(AccessLogger, &AccessLogger::StatsFunc);

void AccessLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	DictionaryData nodes;

	for (const AccessLogger::Ptr& accesslogger : ConfigType::GetObjectsByType<AccessLogger>()) {
		nodes.emplace_back(accesslogger->GetName(), 1); //add more stats
	}

	status->Set("accesslogger", new Dictionary(std::move(nodes)));
}

LogAccess::~LogAccess()
{
	decltype(l_AccessLoggers) loggers;

	{
		boost::mutex::scoped_lock lock (l_AccessLoggersMutex);
		loggers = l_AccessLoggers;
	}

	for (auto& logger : loggers) {
		ObjectLock oLock (logger);
		logger->m_Formatter(*this, *logger->m_Stream);
	}
}

void AccessLogger::Register()
{
	boost::mutex::scoped_lock lock (l_AccessLoggersMutex);
	l_AccessLoggers.insert(this);
}

void AccessLogger::Unregister()
{
	boost::mutex::scoped_lock lock (l_AccessLoggersMutex);
	l_AccessLoggers.erase(this);
}

void AccessLogger::OnAllConfigLoaded()
{
	ObjectImpl<AccessLogger>::OnAllConfigLoaded();

	m_Formatter = ParseFormatter(GetFormat());
}

void AccessLogger::ValidateFormat(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<AccessLogger>::ValidateFormat(lvalue, utils);

	try {
		ParseFormatter(lvalue());
	} catch (const std::invalid_argument& ex) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "format" }, ex.what()));
	}
}

template<class Message>
static inline
void StreamHttpProtocol(const Message& in, std::ostream& out)
{
	auto protocol (in.version());
	auto minor (protocol % 10u);

	out << (protocol / 10u);

	if (minor || protocol != 20u) {
		out << '.' << minor;
	}
}

template<class Message>
static inline
void StreamHttpCLength(const Message& in, std::ostream& out)
{
	if (in.count(http::field::content_length)) {
		out << in[http::field::content_length];
	} else {
		out << '-';
	}
}

template<class Message, class Header>
static inline
void StreamHttpHeader(const Message& in, const Header& header, std::ostream& out)
{
	if (in.count(header)) {
		out << in[header];
	} else {
		out << '-';
	}
}

static boost::regex l_ALFTime (R"EOF(\Atime.(.*)\z)EOF", boost::regex::mod_s);
static boost::regex l_ALFHeaders (R"EOF(\A(request|response).headers.(.*)\z)EOF", boost::regex::mod_s);

static std::unordered_map<std::string, AccessLogger::FormatterFunc*> l_ALFormatters ({
	{ "local.address", [](const LogAccess& in, std::ostream& out) {
		out << in.Stream.lowest_layer().local_endpoint().address();
	} },
	{ "local.port", [](const LogAccess& in, std::ostream& out) {
		out << in.Stream.lowest_layer().local_endpoint().port();
	} },
	{ "remote.address", [](const LogAccess& in, std::ostream& out) {
		out << in.Stream.lowest_layer().remote_endpoint().address();
	} },
	{ "remote.port", [](const LogAccess& in, std::ostream& out) {
		out << in.Stream.lowest_layer().remote_endpoint().port();
	} },
	{ "remote.user", [](const LogAccess& in, std::ostream& out) {
		if (in.User.IsEmpty()) {
			out << '-';
		} else {
			out << in.User;
		}
	} },
	{ "request.method", [](const LogAccess& in, std::ostream& out) {
		out << in.Request.method();
	} },
	{ "request.uri", [](const LogAccess& in, std::ostream& out) {
		out << in.Request.target();
	} },
	{ "request.protocol", [](const LogAccess& in, std::ostream& out) {
		StreamHttpProtocol(in.Request, out);
	} },
	{ "request.size", [](const LogAccess& in, std::ostream& out) {
		StreamHttpCLength(in.Request, out);
	} },
	{ "response.protocol", [](const LogAccess& in, std::ostream& out) {
		StreamHttpProtocol(in.Response, out);
	} },
	{ "response.status", [](const LogAccess& in, std::ostream& out) {
		out << (int)in.Response.result();
	} },
	{ "response.reason", [](const LogAccess& in, std::ostream& out) {
		out << in.Response.reason();
	} },
	{ "response.size", [](const LogAccess& in, std::ostream& out) {
		StreamHttpCLength(in.Response, out);
	} }
});

AccessLogger::Formatter AccessLogger::ParseFormatter(const String& format)
{
	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, format.GetData(), boost::algorithm::is_any_of("$"));

	if (tokens.size() % 2u == 0u) {
		throw std::invalid_argument("Closing $ not found in macro format string '" + format + "'.");
	}

	std::vector<Formatter> formatters;
	std::string literal;
	bool isLiteral = true;

	for (auto& token : tokens) {
		if (isLiteral) {
			literal += token;
		} else if (token.empty()) {
			literal += "$";
		} else {
			if (!literal.empty()) {
				formatters.emplace_back([literal](const LogAccess&, std::ostream& out) {
					out << literal;
				});

				literal = "";
			}

			auto formatter (l_ALFormatters.find(token));

			if (formatter == l_ALFormatters.end()) {
				boost::smatch what;

				if (boost::regex_search(token, what, l_ALFTime)) {
					auto spec (what[1].str());

					formatters.emplace_back([spec](const LogAccess&, std::ostream& out) {
						time_t now;
						struct tm tmNow;

						(void)time(&now);
						(void)localtime_r(&now, &tmNow);

						for (std::vector<char>::size_type size = 64;; size *= 2u) {
							std::vector<char> buf (size);

							if (strftime(buf.data(), size, spec.data(), &tmNow)) {
								out << buf.data();
								break;
							} else if (!strlen(spec.data())) {
								break;
							}
						}
					});
				} else if (boost::regex_search(token, what, l_ALFHeaders)) {
					auto header (what[2].str());

					if (what[1] == "request") {
						formatters.emplace_back([header](const LogAccess& in, std::ostream& out) {
							StreamHttpHeader(in.Request, header, out);
						});
					} else {
						formatters.emplace_back([header](const LogAccess& in, std::ostream& out) {
							StreamHttpHeader(in.Response, header, out);
						});
					}
				} else {
					throw std::invalid_argument("Bad macro '" + token + "'.");
				}
			} else {
				formatters.emplace_back(formatter->second);
			}
		}

		isLiteral = !isLiteral;
	}

	if (!literal.empty()) {
		formatters.emplace_back([literal](const LogAccess&, std::ostream& out) {
			out << literal;
		});
	}

	switch (formatters.size()) {
		case 0u:
			return [](const LogAccess&, std::ostream&) { };
		case 1u:
			return std::move(formatters[0]);
		default:
			return [formatters](const LogAccess& in, std::ostream& out) {
				for (auto& formatter : formatters) {
					formatter(in, out);
				}
			};
	}
}
