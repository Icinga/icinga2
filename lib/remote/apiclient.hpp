/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APICLIENT_H
#define APICLIENT_H

#include "remote/httpclientconnection.hpp"
#include "base/value.hpp"
#include "base/exception.hpp"
#include <vector>

namespace icinga
{

class ApiClient : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ApiClient);

	ApiClient(const String& host, const String& port,
		String user, String password);

	typedef std::function<void(boost::exception_ptr, const Value&)> ExecuteScriptCompletionCallback;
	void ExecuteScript(const String& session, const String& command, bool sandboxed,
		const ExecuteScriptCompletionCallback& callback) const;

	typedef std::function<void(boost::exception_ptr, const Array::Ptr&)> AutocompleteScriptCompletionCallback;
	void AutocompleteScript(const String& session, const String& command, bool sandboxed,
		const AutocompleteScriptCompletionCallback& callback) const;

private:
	HttpClientConnection::Ptr m_Connection;
	String m_User;
	String m_Password;

	static void ExecuteScriptHttpCompletionCallback(HttpRequest& request,
		HttpResponse& response, const ExecuteScriptCompletionCallback& callback);
	static void AutocompleteScriptHttpCompletionCallback(HttpRequest& request,
		HttpResponse& response, const AutocompleteScriptCompletionCallback& callback);
};

}

#endif /* APICLIENT_H */
