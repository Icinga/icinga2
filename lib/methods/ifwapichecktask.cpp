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
#include "base/shared.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsstream.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <exception>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IfwApiCheck, &IfwApiCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

static void ReportIfwCheckResult(
	const Checkable::Ptr& checkable, const CheckCommand::Ptr& command, const CheckResult::Ptr& cr,
	const String& output, int exitcode = 3, const Array::Ptr& perfdata = nullptr
)
{
	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = perfdata ? output + " |" + perfdata->Join(" ") : output;
		pr.ExecutionStart = start;
		pr.ExecutionEnd = end;
		pr.ExitStatus = exitcode;

		Checkable::ExecuteCommandProcessFinishedHandler(command->GetName(), pr);
	} else {
		cr->SetOutput(output);
		cr->SetPerformanceData(perfdata);
		cr->SetState((ServiceState)exitcode);
		cr->SetExitStatus(exitcode);
		cr->SetExecutionStart(start);
		cr->SetExecutionEnd(end);
		cr->SetCommand(command->GetName());

		checkable->ProcessCheckResult(cr);
	}
}

static void ReportIfwCheckResult(
	boost::asio::yield_context yc, const Checkable::Ptr& checkable,
	const CheckCommand::Ptr& command, const CheckResult::Ptr& cr, const String& output
)
{
	CpuBoundWork cbw (yc);

	Utility::QueueAsyncCallback([checkable, command, cr, output]() {
		ReportIfwCheckResult(checkable, command, cr, output);
	});
}

static void ProcessIfwResponse(
	const Checkable::Ptr& checkable, const CheckCommand::Ptr& command, const CheckResult::Ptr& cr, const String& psCommand,
	const String& psHost, const String& psPort, const boost::beast::http::response<boost::beast::http::string_body>& resp
)
{
	Value jsonRoot;

	try {
		jsonRoot = JsonDecode(resp.body());
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Got bad JSON from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what()
		);
		return;
	}

	if (!jsonRoot.IsObjectType<Dictionary>()) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Got JSON, but not an object, from IfW API on host '" + psHost + "' port '" + psPort + "'"
		);
		return;
	}

	Value jsonBranch;

	if (!Dictionary::Ptr(jsonRoot)->Get(psCommand, &jsonBranch)) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Missing ." + psCommand + " in JSON object from IfW API on host '" + psHost + "' port '" + psPort + "'"
		);
		return;
	}

	if (!jsonBranch.IsObjectType<Dictionary>()) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"." + psCommand + " is not an object in JSON from IfW API on host '" + psHost + "' port '" + psPort + "'"
		);
		return;
	}

	Dictionary::Ptr result = jsonBranch;
	double exitcode;

	try {
		exitcode = result->Get("exitcode");
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Got bad exitcode from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what()
		);
		return;
	}

	Array::Ptr perfdata;

	try {
		perfdata = result->Get("perfdata");
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Got bad perfdata from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what()
		);
		return;
	}

	ReportIfwCheckResult(checkable, command, cr, result->Get("checkresult"), exitcode, perfdata);
}

static void DoIfwNetIo(
	boost::asio::yield_context yc, const Checkable::Ptr& checkable, const CheckCommand::Ptr& command,
	const CheckResult::Ptr& cr, const String& psCommand, const String& psHost, const String& psPort,
	const boost::beast::http::request<boost::beast::http::string_body>& req
)
{
	using namespace boost::asio;
	using namespace boost::beast;
	using namespace boost::beast::http;

	ssl::context ctx (ssl::context::tls);
	AsioTlsStream conn (IoEngine::Get().GetIoContext(), ctx);
	flat_buffer buf;
	auto resp (Shared<response<string_body>>::Make())

	try {
		Connect(conn.lowest_layer(), psHost, psPort, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"Can't connect to IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what()
		);
		return;
	}

	try {
		conn.next_layer().async_handshake(conn.next_layer().client, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"TLS handshake with IfW API on host '" + psHost + "' port '" + psPort + "' failed: " + ex.what()
		);
		return;
	}

	double start = Utility::GetTime();

	try {
		async_write(conn, req, yc);
		conn.async_flush(yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"Can't send HTTP request to IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what()
		);
		return;
	}

	try {
		async_read(conn, buf, *resp, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"Can't read HTTP response from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what()
		);
		return;
	}

	double end = Utility::GetTime();
	CpuBoundWork cbw (yc);

	Utility::QueueAsyncCallback([checkable, command, cr, psCommand, psHost, psPort, resp]() {
		ProcessIfwResponse(checkable, command, cr, psCommand, psHost, psPort, *resp);
	});
}

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

	String psHost = MacroProcessor::ResolveMacros("$ifw_api_host$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String psPort = MacroProcessor::ResolveMacros("$ifw_api_port$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	Dictionary::Ptr params = new Dictionary();

	if (arguments) {
		ObjectLock oLock (arguments);
		Array::Ptr emptyCmd = new Array();

		for (auto& kv : arguments) {
			/* MacroProcessor::ResolveArguments() converts
			 *
			 * [ "check_example" ]
			 * and
			 * {
			 * 	 "-f" = { set_if = "$example_flag$" }
			 * 	 "-a" = "$example_arg$"
			 * }
			 *
			 * to
			 *
			 * [ "check_example", "-f", "-a", "X" ]
			 *
			 * but we need the args one-by-one like [ "-f" ] or [ "-a", "X" ].
			 */
			Array::Ptr arg = MacroProcessor::ResolveArguments(
				emptyCmd, new Dictionary({{kv.first, kv.second}}), resolvers,
				checkable->GetLastCheckResult(), resolvedMacros, useResolvedMacros
			);

			switch (arg ? arg->GetLength() : 0) {
				case 0:
					break;
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

	auto req (Shared<request<string_body>>::Make());

	req->method(verb::post);
	req->target("/v1/checker?command=" + psCommand);
	req->set(field::content_type, "application/json");
	req->body() = JsonEncode(params);

	IoEngine::SpawnCoroutine(
		IoEngine::Get().GetIoContext(),
		[checkable, command, cr, psCommand, psHost, psPort, req](asio::yield_context yc) {
			DoIfwNetIo(yc, checkable, command, cr, psCommand, psHost, psPort, *req);
		}
	);
}
