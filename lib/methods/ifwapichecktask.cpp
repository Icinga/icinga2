/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/ifwapichecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/pluginutility.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsstream.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IfwApiCheck, &IfwApiCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void IfwApiCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	using namespace boost::asio;
	using namespace boost::beast;
	using namespace boost::beast::http;

	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	CheckCommand::Ptr command = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;

	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", command);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String psCommand = MacroProcessor::ResolveMacros("$ifw_api_command$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	Dictionary::Ptr arguments = MacroProcessor::ResolveMacros("$ifw_api_arguments$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	Dictionary::Ptr params = new Dictionary();

	if (arguments) {
		ObjectLock oLock (arguments);
		Array::Ptr dummy = new Array();

		for (auto& kv : arguments) {
			Array::Ptr arg = MacroProcessor::ResolveArguments(
				dummy, new Dictionary({{kv.first, kv.second}}), resolvers,
				checkable->GetLastCheckResult(), resolvedMacros, useResolvedMacros
			);

			switch (arg ? arg->GetLength() : 0) {
				case 0:
					continue;
				case 1:
					params->Set(arg->Get(0), true);
					break;
				case 2:
					params->Set(arg->Get(0), arg->Get(1));
					break;
				default: {
					auto k (arg->Get(0));

					arg->Remove(0);
					params->Set(k, arg);
				}
			}
		}
	}

	if (resolvedMacros && !useResolvedMacros)
		return;

	ssl::context ctx (ssl::context::tls);
	AsioTlsStream conn (IoEngine::Get().GetIoContext(), ctx);
	request<string_body> req;
	flat_buffer buf;
	response<string_body> resp;

	req.method(verb::post);
	req.target("/v1/checker?command=" + psCommand);
	req.set(field::content_type, "application/json");
	req.body() = JsonEncode(params);

	Connect(conn.lowest_layer(), "127.0.0.1", "5668");
	conn.next_layer().handshake(conn.next_layer().client);

	double start = Utility::GetTime();

	write(conn, req);
	conn.flush();
	read(conn, buf, resp);

	double end = Utility::GetTime();

	Dictionary::Ptr result = Dictionary::Ptr(JsonDecode(resp.body()))->Get(psCommand);

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = result->Get("checkresult") + " |" + Array::Ptr(result->Get("perfdata"))->Join("");
		pr.ExecutionStart = start;
		pr.ExecutionEnd = end;
		pr.ExitStatus = result->Get("exitcode");

		Checkable::ExecuteCommandProcessFinishedHandler(command->GetName(), pr);
	} else {
		cr->SetOutput(result->Get("checkresult"));
		cr->SetPerformanceData(result->Get("perfdata"));
		cr->SetState((ServiceState)(int)result->Get("exitcode"));
		cr->SetExitStatus(result->Get("exitcode"));
		cr->SetExecutionStart(start);
		cr->SetExecutionEnd(end);
		cr->SetCommand(command->GetName());

		checkable->ProcessCheckResult(cr);
	}
}
