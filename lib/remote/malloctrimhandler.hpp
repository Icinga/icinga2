// SPDX-FileCopyrightText: 2024 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "remote/httphandler.hpp"

namespace icinga
{

class MallocTrimHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(MallocTrimHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}
