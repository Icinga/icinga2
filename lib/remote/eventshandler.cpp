/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/eventshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/defer.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

REGISTER_URLHANDLER("/v1/events", EventsHandler);

bool EventsHandler::HandleRequest(
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context& yc,
	bool& hasStartedStreaming
)
{
	namespace asio = boost::asio;
	namespace http = boost::beast::http;

	if (url->GetPath().size() != 2)
		return false;

	if (request.method() != http::verb::post)
		return false;

	if (request.version() == 10) {
		HttpUtility::SendJsonError(response, params, 400, "HTTP/1.0 not supported for event streams.");
		return true;
	}

	Array::Ptr types = params->Get("types");

	if (!types) {
		HttpUtility::SendJsonError(response, params, 400, "'types' query parameter is required.");
		return true;
	}

	{
		ObjectLock olock(types);
		for (const String& type : types) {
			FilterUtility::CheckPermission(user, "events/" + type);
		}
	}

	String queueName = HttpUtility::GetLastParameter(params, "queue");

	if (queueName.IsEmpty()) {
		HttpUtility::SendJsonError(response, params, 400, "'queue' query parameter is required.");
		return true;
	}

	String filter = HttpUtility::GetLastParameter(params, "filter");

	std::unique_ptr<Expression> ufilter;

	if (!filter.IsEmpty())
		ufilter = ConfigCompiler::CompileText("<API query>", filter);

	/* create a new queue or update an existing one */
	EventQueue::Ptr queue = EventQueue::GetByName(queueName);

	if (!queue) {
		queue = new EventQueue(queueName);
		EventQueue::Register(queueName, queue);
	}

	queue->SetTypes(types->ToSet<String>());
	queue->SetFilter(std::move(ufilter));

	queue->AddClient(&request);

	Defer removeClient ([&queue, &request, &queueName]() {
		queue->RemoveClient(&request);
		EventQueue::UnregisterIfUnused(queueName, queue);
	});

	hasStartedStreaming = true;

	response.result(http::status::ok);
	response.set(http::field::content_type, "application/json");

	http::async_write(stream, response, yc);
	stream.async_flush(yc);

	asio::const_buffer newLine ("\n", 1);

	for (;;) {
		Dictionary::Ptr result = queue->WaitForEvent(&request);

		if (!result)
			continue;

		String body = JsonEncode(result);

		boost::algorithm::replace_all(body, "\n", "");

		asio::const_buffer payload (body.CStr(), body.GetLength());

		stream.async_write_some(payload, yc);
		stream.async_write_some(newLine, yc);
		stream.async_flush(yc);
	}
}

