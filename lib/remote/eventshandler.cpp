/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/eventshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <map>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/events", EventsHandler);

const std::map<String, EventType> l_EventTypes ({
	{"AcknowledgementCleared", EventType::AcknowledgementCleared},
	{"AcknowledgementSet", EventType::AcknowledgementSet},
	{"CheckResult", EventType::CheckResult},
	{"CommentAdded", EventType::CommentAdded},
	{"CommentRemoved", EventType::CommentRemoved},
	{"DowntimeAdded", EventType::DowntimeAdded},
	{"DowntimeRemoved", EventType::DowntimeRemoved},
	{"DowntimeStarted", EventType::DowntimeStarted},
	{"DowntimeTriggered", EventType::DowntimeTriggered},
	{"Flapping", EventType::Flapping},
	{"Notification", EventType::Notification},
	{"StateChange", EventType::StateChange},
	{"ObjectCreated", EventType::ObjectCreated},
	{"ObjectDeleted", EventType::ObjectDeleted},
	{"ObjectModified", EventType::ObjectModified}
});

const String l_ApiQuery ("<API query>");

bool EventsHandler::HandleRequest(
	const WaitGroup::Ptr&,
	AsioTlsStream& stream,
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	namespace asio = boost::asio;
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

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
		for (String type : types) {
			FilterUtility::CheckPermission(user, "events/" + type);
		}
	}

	String queueName = HttpUtility::GetLastParameter(params, "queue");

	if (queueName.IsEmpty()) {
		HttpUtility::SendJsonError(response, params, 400, "'queue' query parameter is required.");
		return true;
	}

	std::set<EventType> eventTypes;

	{
		ObjectLock olock(types);
		for (String type : types) {
			auto typeId (l_EventTypes.find(type));

			if (typeId != l_EventTypes.end()) {
				eventTypes.emplace(typeId->second);
			}
		}
	}

	EventsSubscriber subscriber (std::move(eventTypes), HttpUtility::GetLastParameter(params, "filter"), l_ApiQuery);

	server.StartStreaming();

	response.result(http::status::ok);
	response.set(http::field::content_type, "application/json");

	IoBoundWorkSlot dontLockTheIoThread (yc);

	http::async_write(stream, response, yc);
	stream.async_flush(yc);

	asio::const_buffer newLine ("\n", 1);

	for (;;) {
		auto event (subscriber.GetInbox()->Shift(yc));

		if (event) {
			String body = JsonEncode(event);

			boost::algorithm::replace_all(body, "\n", "");

			asio::const_buffer payload (body.CStr(), body.GetLength());

			asio::async_write(stream, payload, yc);
			asio::async_write(stream, newLine, yc);
			stream.async_flush(yc);
		} else if (server.Disconnected()) {
			return true;
		}
	}
}
