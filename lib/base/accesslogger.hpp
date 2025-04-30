/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef ACCESSLOGGER_H
#define ACCESSLOGGER_H

#include "base/i2-base.hpp"
#include "base/accesslogger-ti.hpp"
#include "base/shared.hpp"
#include "base/tlsstream.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <functional>
#include <utility>

namespace icinga
{

class LogAccess
{
public:
	inline LogAccess(
		const AsioTlsStream& stream,
		const boost::beast::http::request<boost::beast::http::string_body>& request,
		String user,
		const boost::beast::http::response<boost::beast::http::string_body>& response
	) : Stream(stream), Request(request), User(std::move(user)), Response(response)
	{}

	LogAccess(const LogAccess&) = delete;
	LogAccess(LogAccess&&) = delete;
	LogAccess& operator=(const LogAccess&) = delete;
	LogAccess& operator=(LogAccess&&) = delete;
	~LogAccess();

	const AsioTlsStream& Stream;
	const boost::beast::http::request<boost::beast::http::string_body>& Request;
	String User;
	const boost::beast::http::response<boost::beast::http::string_body>& Response;
};

/**
 * A file logger that logs API access.
 *
 * @ingroup base
 */
class AccessLogger final : public ObjectImpl<AccessLogger>
{
	friend LogAccess;

public:
	typedef void FormatterFunc(const LogAccess& in, std::ostream& out);
	typedef std::function<FormatterFunc> Formatter;

	DECLARE_OBJECT(AccessLogger);
	DECLARE_OBJECTNAME(AccessLogger);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	void OnAllConfigLoaded() override;
	void ValidateFormat(const Lazy<String>& lvalue, const ValidationUtils& utils) override;
	void Register() override;
	void Unregister() override;

private:
	Formatter ParseFormatter(const String& format);

	Formatter m_Formatter;
};

}

#endif /* ACCESSLOGGER_H */
