/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGSTAGESHANDLER_H
#define CONFIGSTAGESHANDLER_H

#include "remote/httphandler.hpp"
#include <atomic>

namespace icinga
{

class ConfigStagesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigStagesHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		AsioTlsStream& stream,
		HttpRequest& request,
		HttpResponse& response,
		boost::asio::yield_context& yc,
		HttpServerConnection& server
	) override;

private:
	void HandleGet(HttpRequest& request, HttpResponse& response);
	void HandlePost(HttpRequest& request, HttpResponse& response);
	void HandleDelete(HttpRequest& request, HttpResponse& response);

	static std::atomic<bool> m_RunningPackageUpdates;
};

}

#endif /* CONFIGSTAGESHANDLER_H */
