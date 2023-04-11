/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/ifwapichecktask.hpp"
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
#include <exception>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IfwApiCheck, &IfwApiCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

static void ReportIfwCheckResult(
	const Checkable::Ptr& checkable, const CheckCommand::Ptr& command, const CheckResult::Ptr& cr, const String& output,
	double start, double end, int exitcode = 3, const Array::Ptr& perfdata = nullptr
)
{
	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = perfdata ? output + " |" + String(perfdata->Join(" ")) : output;
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
	boost::asio::yield_context yc, const Checkable::Ptr& checkable, const CheckCommand::Ptr& command,
	const CheckResult::Ptr& cr, const String& output, double start
)
{
	double end = Utility::GetTime();
	CpuBoundWork cbw (yc);

	Utility::QueueAsyncCallback([checkable, command, cr, output, start, end]() {
		ReportIfwCheckResult(checkable, command, cr, output, start, end);
	});
}

static void DoIfwNetIo(
	boost::asio::yield_context yc, const Checkable::Ptr& checkable, const CheckCommand::Ptr& command,
	const CheckResult::Ptr& cr, const String& psCommand, const String& psHost, const String& sni, const String& psPort,
	AsioTlsStream& conn, boost::beast::http::request<boost::beast::http::string_body>& req, double start
)
{
	using namespace boost::asio;
	using namespace boost::beast;
	using namespace boost::beast::http;

	flat_buffer buf;
	response<string_body> resp;

	try {
		Connect(conn.lowest_layer(), psHost, psPort, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"Can't connect to IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start
		);
		return;
	}

	auto& sslConn (conn.next_layer());

	try {
		sslConn.async_handshake(conn.next_layer().client, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"TLS handshake with IfW API on host '" + psHost + "' (SNI: '" + sni
				+ "') port '" + psPort + "' failed: " + ex.what(), start
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
			yc, checkable, command, cr,
			"Certificate validation failed for IfW API on host '" + psHost + "' (SNI: '" + sni + "'; CN: "
				+ (cn.IsString() ? "'" + cn + "'" : "N/A") + ") port '" + psPort + "': " + sslConn.GetVerifyError(),
			start
		);
		return;
	}

	try {
		async_write(conn, req, yc);
		conn.async_flush(yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"Can't send HTTP request to IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start
		);
		return;
	}

	try {
		async_read(conn, buf, resp, yc);
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			yc, checkable, command, cr,
			"Can't read HTTP response from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start
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
			checkable, command, cr,
			"Got bad JSON from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start, end
		);
		return;
	}

	if (!jsonRoot.IsObjectType<Dictionary>()) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Got JSON, but not an object, from IfW API on host '" + psHost + "' port '" + psPort + "'", start, end
		);
		return;
	}

	Value jsonBranch;

	if (!Dictionary::Ptr(jsonRoot)->Get(psCommand, &jsonBranch)) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Missing ." + psCommand + " in JSON object from IfW API on host '" + psHost + "' port '" + psPort + "'", start, end
		);
		return;
	}

	if (!jsonBranch.IsObjectType<Dictionary>()) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"." + psCommand + " is not an object in JSON from IfW API on host '" + psHost + "' port '" + psPort + "'", start, end
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
			"Got bad exitcode from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start, end
		);
		return;
	}

	Array::Ptr perfdata;

	try {
		perfdata = result->Get("perfdata");
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(
			checkable, command, cr,
			"Got bad perfdata from IfW API on host '" + psHost + "' port '" + psPort + "': " + ex.what(), start, end
		);
		return;
	}

	ReportIfwCheckResult(checkable, command, cr, result->Get("checkresult"), start, end, exitcode, perfdata);
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

	auto resolveMacros ([&resolvers, &lcr, &resolvedMacros, useResolvedMacros](const char* macros, String* missingMacro = nullptr) -> Value {
		return MacroProcessor::ResolveMacros(
			macros, resolvers, lcr, missingMacro, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros
		);
	});

	String missingCert, missingKey, missingCa, missingCrl, missingUsername, missingPassword;

	String psCommand = resolveMacros("$ifw_api_command$");
	Dictionary::Ptr arguments = resolveMacros("$ifw_api_arguments$");
	Array::Ptr ignoreArguments = resolveMacros("$ifw_api_ignore_arguments$");
	String psHost = resolveMacros("$ifw_api_host$");
	String psPort = resolveMacros("$ifw_api_port$");
	String sni = resolveMacros("$ifw_api_sni$");
	Array::Ptr sniDenylist = resolveMacros("$ifw_api_sni_denylist$");
	String cert = resolveMacros("$ifw_api_cert$", &missingCert);
	String key = resolveMacros("$ifw_api_key$", &missingKey);
	String ca = resolveMacros("$ifw_api_ca$", &missingCa);
	String crl = resolveMacros("$ifw_api_crl$", &missingCrl);
	String username = resolveMacros("$ifw_api_username$", &missingUsername);
	String password = resolveMacros("$ifw_api_password$", &missingPassword);

	Dictionary::Ptr params = new Dictionary();

	if (arguments) {
		auto ignore (ignoreArguments->ToSet<String>());
		ObjectLock oLock (arguments);
		Array::Ptr emptyCmd = new Array();

		for (auto& kv : arguments) {
			if (ignore.find(kv.first) != ignore.end()) {
				continue;
			}

			Dictionary::Ptr argSpec;

			if (kv.second.IsObjectType<Dictionary>()) {
				argSpec = Dictionary::Ptr(kv.second)->ShallowClone();
			} else {
				argSpec = new Dictionary({{ "value", kv.second }});
			}

			// See default branch of below switch
			argSpec->Set("repeat_key", false);

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

	if (sniDenylist->Contains(sni)) {
		sni = IcingaApplication::GetInstance()->GetNodeName();
	}

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (!missingCert.IsEmpty()) {
		cert = ApiListener::GetDefaultCertPath();
	}

	if (!missingKey.IsEmpty()) {
		key = ApiListener::GetDefaultKeyPath();
	}

	if (!missingCa.IsEmpty()) {
		ca = ApiListener::GetDefaultCaPath();
	}

	Url::Ptr uri = new Url();

	uri->SetPath({ "v1", "checker" });
	uri->SetQuery({{ "command", psCommand }});

	auto req (Shared<request<string_body>>::Make());

	req->method(verb::post);
	req->target(uri->Format());
	req->set(field::content_type, "application/json");
	req->body() = JsonEncode(params);

	if (missingUsername.IsEmpty() && missingPassword.IsEmpty()) {
		req->set(field::authorization, "Basic " + Base64::Encode(username + ":" + password));
	}

	auto& io (IoEngine::Get().GetIoContext());
	auto strand (Shared<boost::asio::io_context::strand>::Make(io));
	Shared<boost::asio::ssl::context>::Ptr ctx;
	double start = Utility::GetTime();

	try {
		ctx = SetupSslContext(cert, key, ca, crl, DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
	} catch (const std::exception& ex) {
		ReportIfwCheckResult(checkable, command, cr, ex.what(), start, Utility::GetTime());
		return;
	}

	auto conn (Shared<AsioTlsStream>::Make(io, *ctx, sni));

	IoEngine::SpawnCoroutine(
		*strand,
		[strand, checkable, command, cr, psCommand, psHost, sni, psPort, conn, req, start, checkTimeout](boost::asio::yield_context yc) {
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

			DoIfwNetIo(yc, checkable, command, cr, psCommand, psHost, sni, psPort, *conn, *req, start);
		}
	);
}
