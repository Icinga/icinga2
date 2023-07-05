/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/ifwapichecktask.hpp"
#include "methods/pluginchecktask.hpp"
#include "icinga/checkresult-ti.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/pluginutility.hpp"
#include "base/base64.hpp"
#include "base/defer.hpp"
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
#include "remote/apilistener.hpp"
#include "remote/url.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/system/system_error.hpp>
#include <exception>
#include <set>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IfwApiCheck, &IfwApiCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

static void ReportIfwCheckResult(
	const Checkable::Ptr& checkable, const Value& cmdLine, const CheckResult::Ptr& cr,
	const String& output, double start, double end, int exitcode = 3, const Array::Ptr& perfdata = nullptr
)
{
	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = perfdata ? output + " |" + String(perfdata->Join(" ")) : output;
		pr.ExecutionStart = start;
		pr.ExecutionEnd = end;
		pr.ExitStatus = exitcode;

		Checkable::ExecuteCommandProcessFinishedHandler(cmdLine, pr);
	} else {
		auto splittedPerfdata (perfdata);

		if (perfdata) {
			splittedPerfdata = new Array();
			ObjectLock oLock (perfdata);

			for (String pv : perfdata) {
				PluginUtility::SplitPerfdata(pv)->CopyTo(splittedPerfdata);
			}
		}

		cr->SetOutput(output);
		cr->SetPerformanceData(splittedPerfdata);
		cr->SetState((ServiceState)exitcode);
		cr->SetExitStatus(exitcode);
		cr->SetExecutionStart(start);
		cr->SetExecutionEnd(end);
		cr->SetCommand(cmdLine);

		checkable->ProcessCheckResult(cr);
	}
}

static void ReportIfwCheckResult(
	boost::asio::yield_context yc, const Checkable::Ptr& checkable, const Value& cmdLine,
	const CheckResult::Ptr& cr, const String& output, double start
)
{
	double end = Utility::GetTime();
	CpuBoundWork cbw (yc);

	ReportIfwCheckResult(checkable, cmdLine, cr, output, start, end);
}

static const char* GetUnderstandableError(const std::exception& ex)
{
	auto se (dynamic_cast<const boost::system::system_error*>(&ex));

	if (se && se->code() == boost::asio::error::operation_aborted) {
		return "Timeout exceeded";
	}

	return ex.what();
}

static void DoIfwNetIo(
	boost::asio::yield_context yc, const Checkable::Ptr& checkable, const Array::Ptr& cmdLine,
	const CheckResult::Ptr& cr, const String& psCommand, const String& psHost, const String& san, const String& psPort,
	AsioTlsStream& conn, boost::beast::http::request<boost::beast::http::string_body>& req, double start
)
{
	namespace http = boost::beast::http;

	boost::beast::flat_buffer buf;
	http::response<http::string_body> resp;

	try {
		Connect(conn.lowest_layer(), psHost, psPort, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, cmdLine, cr,
			"Can't connect to IfW API on host '" + psHost + "' port '" + psPort + "': " + GetUnderstandableError(ex),
			start
		);
		return;
	}

	auto& sslConn (conn.next_layer());

	try {
		sslConn.async_handshake(conn.next_layer().client, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, cmdLine, cr,
			"TLS handshake with IfW API on host '" + psHost + "' (SNI: '" + san
				+ "') port '" + psPort + "' failed: " + GetUnderstandableError(ex),
			start
		);
		return;
	}

	if (!sslConn.IsVerifyOK()) {
		auto cert (sslConn.GetPeerCertificate());
		Value cn;

		try {
			cn = GetCertificateCN(cert);
		} catch (const std::exception&) {
		}

		ReportIfwCheckResult(
			yc, checkable, cmdLine, cr,
			"Certificate validation failed for IfW API on host '" + psHost + "' (SNI: '" + san + "'; CN: "
				+ (cn.IsString() ? "'" + cn + "'" : "N/A") + ") port '" + psPort + "': " + sslConn.GetVerifyError(),
			start
		);
		return;
	}

	try {
		http::async_write(conn, req, yc);
		conn.async_flush(yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, cmdLine, cr,
			"Can't send HTTP request to IfW API on host '" + psHost + "' port '" + psPort + "': " + GetUnderstandableError(ex),
			start
		);
		return;
	}

	try {
		http::async_read(conn, buf, resp, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, cmdLine, cr,
			"Can't read HTTP response from IfW API on host '" + psHost + "' port '" + psPort + "': " + GetUnderstandableError(ex),
			start
		);
		return;
	}

	double end = Utility::GetTime();

	{
		boost::system::error_code ec;
		sslConn.async_shutdown(yc[ec]);
	}

	CpuBoundWork cbw (yc);
	Value jsonRoot;

	try {
		jsonRoot = JsonDecode(resp.body());
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"Got bad JSON from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start, end
		);
		return;
	}

	if (!jsonRoot.IsObjectType<Dictionary>()) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"Got JSON, but not an object, from IfW API on host '"
				+ psHost + "' port '" + psPort + "': " + JsonEncode(jsonRoot),
			start, end
		);
		return;
	}

	Value jsonBranch;

	if (!Dictionary::Ptr(jsonRoot)->Get(psCommand, &jsonBranch)) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"Missing ." + psCommand + " in JSON object from IfW API on host '"
				+ psHost + "' port '" + psPort + "': " + JsonEncode(jsonRoot),
			start, end
		);
		return;
	}

	if (!jsonBranch.IsObjectType<Dictionary>()) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"." + psCommand + " in JSON from IfW API on host '"
				+ psHost + "' port '" + psPort + "' is not an object: " + JsonEncode(jsonBranch),
			start, end
		);
		return;
	}

	Dictionary::Ptr result = jsonBranch;

	Value exitcode;

	if (!result->Get("exitcode", &exitcode)) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"Missing ." + psCommand + ".exitcode in JSON object from IfW API on host '"
				+ psHost + "' port '" + psPort + "': " + JsonEncode(result),
			start, end
		);
		return;
	}

	static const std::set<double> exitcodes {ServiceOK, ServiceWarning, ServiceCritical, ServiceUnknown};
	static const auto exitcodeList (Array::FromSet(exitcodes)->Join(", "));

	if (!exitcode.IsNumber() || exitcodes.find(exitcode) == exitcodes.end()) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"Got bad exitcode " + JsonEncode(exitcode) + " from IfW API on host '" + psHost + "' port '" + psPort
				+ "', expected one of: " + exitcodeList,
			start, end
		);
		return;
	}

	auto perfdataVal (result->Get("perfdata"));
	Array::Ptr perfdata;

	try {
		perfdata = perfdataVal;
	} catch (const std::exception&) {
		ReportIfwCheckResult(
			checkable, cmdLine, cr,
			"Got bad perfdata " + JsonEncode(perfdataVal) + " from IfW API on host '"
				+ psHost + "' port '" + psPort + "', expected an array",
			start, end
		);
		return;
	}

	if (perfdata) {
		ObjectLock oLock (perfdata);

		for (auto& pv : perfdata) {
			if (!pv.IsString()) {
				ReportIfwCheckResult(
					checkable, cmdLine, cr,
					"Got bad perfdata value " + JsonEncode(perfdata) + " from IfW API on host '"
						+ psHost + "' port '" + psPort + "', expected an array of strings",
					start, end
				);
				return;
			}
		}
	}

	ReportIfwCheckResult(checkable, cmdLine, cr, result->Get("checkresult"), start, end, exitcode, perfdata);
}

void IfwApiCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	namespace asio = boost::asio;
	namespace http = boost::beast::http;
	using http::field;

	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	// We're going to just resolve macros for the actual check execution happening elsewhere
	if (resolvedMacros && !useResolvedMacros) {
		auto commandEndpoint (checkable->GetCommandEndpoint());

		// There's indeed a command endpoint, obviously for the actual check execution
		if (commandEndpoint) {
			// But it doesn't have this function, yet ("ifw-api-check-command")
			if (!(commandEndpoint->GetCapabilities() & (uint_fast64_t)ApiCapabilities::IfwApiCheckCommand)) {
				// Assume "ifw-api-check-command" has been imported into a check command which can also work
				// based on "plugin-check-command", delegate respectively and hope for the best
				PluginCheckTask::ScriptFunc(checkable, cr, resolvedMacros, useResolvedMacros);
				return;
			}
		}
	}

	CheckCommand::Ptr command = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();
	auto lcr (checkable->GetLastCheckResult());

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

	auto resolveMacros ([&resolvers, &lcr, &resolvedMacros, useResolvedMacros](const char* macros) -> Value {
		return MacroProcessor::ResolveMacros(
			macros, resolvers, lcr, nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros
		);
	});

	String psCommand = resolveMacros("$ifw_api_command$");
	Dictionary::Ptr arguments = resolveMacros("$ifw_api_arguments$");
	String psHost = resolveMacros("$ifw_api_host$");
	String psPort = resolveMacros("$ifw_api_port$");
	String expectedSan = resolveMacros("$ifw_api_expected_san$");
	String cert = resolveMacros("$ifw_api_cert$");
	String key = resolveMacros("$ifw_api_key$");
	String ca = resolveMacros("$ifw_api_ca$");
	String crl = resolveMacros("$ifw_api_crl$");
	String username = resolveMacros("$ifw_api_username$");
	String password = resolveMacros("$ifw_api_password$");

	Dictionary::Ptr params = new Dictionary();

	if (arguments) {
		ObjectLock oLock (arguments);
		Array::Ptr emptyCmd = new Array();

		for (auto& kv : arguments) {
			Dictionary::Ptr argSpec;

			if (kv.second.IsObjectType<Dictionary>()) {
				argSpec = Dictionary::Ptr(kv.second)->ShallowClone();
			} else {
				argSpec = new Dictionary({{ "value", kv.second }});
			}

			// See default branch of below switch
			argSpec->Set("repeat_key", false);

			{
				ObjectLock oLock (argSpec);

				for (auto& kv : argSpec) {
					if (kv.second.GetType() == ValueObject) {
						auto now (Utility::GetTime());

						ReportIfwCheckResult(
							checkable, command->GetName(), cr,
							"$ifw_api_arguments$ may not directly contain objects (especially functions).", now, now
						);

						return;
					}
				}
			}

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
				emptyCmd, new Dictionary({{kv.first, argSpec}}), resolvers, lcr, resolvedMacros, useResolvedMacros
			);

			switch (arg ? arg->GetLength() : 0) {
				case 0:
					break;
				case 1: // [ "-f" ]
					params->Set(arg->Get(0), true);
					break;
				case 2: // [ "-a", "X" ]
					params->Set(arg->Get(0), arg->Get(1));
					break;
				default: { // [ "-a", "X", "Y" ]
					auto k (arg->Get(0));

					arg->Remove(0);
					params->Set(k, arg);
				}
			}
		}
	}

	auto checkTimeout (command->GetTimeout());
	auto checkableTimeout (checkable->GetCheckTimeout());

	if (!checkableTimeout.IsEmpty())
		checkTimeout = checkableTimeout;

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (psHost.IsEmpty()) {
		psHost = "localhost";
	}

	if (expectedSan.IsEmpty()) {
		expectedSan = IcingaApplication::GetInstance()->GetNodeName();
	}

	if (cert.IsEmpty()) {
		cert = ApiListener::GetDefaultCertPath();
	}

	if (key.IsEmpty()) {
		key = ApiListener::GetDefaultKeyPath();
	}

	if (ca.IsEmpty()) {
		ca = ApiListener::GetDefaultCaPath();
	}

	Url::Ptr uri = new Url();

	uri->SetPath({ "v1", "checker" });
	uri->SetQuery({{ "command", psCommand }});

	static const auto userAgent ("Icinga/" + Application::GetAppVersion());
	auto relative (uri->Format());
	auto body (JsonEncode(params));
	auto req (Shared<http::request<http::string_body>>::Make());

	req->method(http::verb::post);
	req->target(relative);
	req->set(field::accept, "application/json");
	req->set(field::content_type, "application/json");
	req->set(field::host, expectedSan + ":" + psPort);
	req->set(field::user_agent, userAgent);
	req->body() = body;
	req->content_length(req->body().size());

	static const auto curlTlsMinVersion ((String("--") + DEFAULT_TLS_PROTOCOLMIN).ToLower());

	Array::Ptr cmdLine = new Array({
		"curl", "--verbose", curlTlsMinVersion, "--fail-with-body",
		"--connect-to", expectedSan + ":" + psPort + ":" + psHost + ":" + psPort,
		"--ciphers", DEFAULT_TLS_CIPHERS,
		"--cert", cert,
		"--key", key,
		"--cacert", ca,
		"--request", "POST",
		"--url", "https://" + expectedSan + ":" + psPort + relative,
		"--user-agent", userAgent,
		"--header", "Accept: application/json",
		"--header", "Content-Type: application/json",
		"--data-raw", body
	});

	if (!crl.IsEmpty()) {
		cmdLine->Add("--crlfile");
		cmdLine->Add(crl);
	}

	if (!username.IsEmpty() && !password.IsEmpty()) {
		auto authn (username + ":" + password);

		req->set(field::authorization, "Basic " + Base64::Encode(authn));
		cmdLine->Add("--user");
		cmdLine->Add(authn);
	}

	auto& io (IoEngine::Get().GetIoContext());
	auto strand (Shared<asio::io_context::strand>::Make(io));
	Shared<asio::ssl::context>::Ptr ctx;
	double start = Utility::GetTime();

	try {
		ctx = SetupSslContext(cert, key, ca, crl, DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(checkable, cmdLine, cr, ex.what(), start, Utility::GetTime());
		return;
	}

	auto conn (Shared<AsioTlsStream>::Make(io, *ctx, expectedSan));

	IoEngine::SpawnCoroutine(
		*strand,
		[strand, checkable, cmdLine, cr, psCommand, psHost, expectedSan, psPort, conn, req, start, checkTimeout](asio::yield_context yc) {
			Timeout::Ptr timeout = new Timeout(strand->context(), *strand, boost::posix_time::microseconds(int64_t(checkTimeout * 1e6)),
				[&conn, &checkable](boost::asio::yield_context yc) {
					Log(LogNotice, "IfwApiCheckTask")
						<< "Timeout while checking " << checkable->GetReflectionType()->GetName()
						<< " '" << checkable->GetName() << "', cancelling attempt";

					boost::system::error_code ec;
					conn->lowest_layer().cancel(ec);
				}
			);

			Defer cancelTimeout ([&timeout]() { timeout->Cancel(); });

			DoIfwNetIo(yc, checkable, cmdLine, cr, psCommand, psHost, expectedSan, psPort, *conn, *req, start);
		}
	);
}
