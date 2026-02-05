// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGMODULESHANDLER_H
#define CONFIGMODULESHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigPackagesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigPackagesHandler);

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

#endif /* CONFIGMODULESHANDLER_H */
