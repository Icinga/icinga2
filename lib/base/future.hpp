/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef FUTURE_H
#define FUTURE_H

#include "base/i2-base.hpp"
#include "base/future.thpp"
#include "base/value.hpp"
#include "base/function.hpp"
#include <future>

namespace icinga
{

/**
 * A future value.
 *
 * @ingroup base
 */
class Future : public ObjectImpl<Future>
{
public:
	DECLARE_OBJECT(Future);

	Future(std::future<Value>&& fut);

	Value Get(void);
	Future::Ptr ContinueWith(const Function::Ptr& callback);

	static Object::Ptr GetPrototype(void);

private:
	std::future<Value> m_Future;
};

}

#endif /* FUTURE_H */
