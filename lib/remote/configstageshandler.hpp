// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGSTAGESHANDLER_H
#define CONFIGSTAGESHANDLER_H

#include "base/atomic.hpp"
#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigStagesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigStagesHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;

private:
	void HandleGet(const HttpApiRequest& request, HttpApiResponse& response, boost::asio::yield_context& yc);
	void HandlePost(const HttpApiRequest& request, HttpApiResponse& response);
	void HandleDelete(const HttpApiRequest& request, HttpApiResponse& response);
};

}

#endif /* CONFIGSTAGESHANDLER_H */
