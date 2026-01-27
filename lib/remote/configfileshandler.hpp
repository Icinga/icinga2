// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGFILESHANDLER_H
#define CONFIGFILESHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ConfigFilesHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigFilesHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}

#endif /* CONFIGFILESHANDLER_H */
