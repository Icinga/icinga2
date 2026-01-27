// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TYPEQUERYHANDLER_H
#define TYPEQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class TypeQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(TypeQueryHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;
};

}

#endif /* TYPEQUERYHANDLER_H */
