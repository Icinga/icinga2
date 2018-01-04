/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef OBJECTQUERYHANDLER_H
#define OBJECTQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ObjectQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ObjectQueryHandler);

	bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request,
		HttpResponse& response, const Dictionary::Ptr& params) override;

private:
	static Dictionary::Ptr SerializeObjectAttrs(const Object::Ptr& object, const String& attrPrefix,
		const Array::Ptr& attrs, bool isJoin, bool allAttrs);
};

}

#endif /* OBJECTQUERYHANDLER_H */
