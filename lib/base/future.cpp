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

#include "base/future.hpp"
#include "base/future.tcpp"
#include "base/scriptutils.hpp"

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(Future, Future::GetPrototype());

Future::Future(std::future<Value>&& fut)
    : m_Future(std::move(fut))
{ }

Value Future::Get(void)
{
	if (!m_Future.valid())
		BOOST_THROW_EXCEPTION(ScriptError("Future is not valid."));

	return m_Future.get();
}

static Value FutureContinueWithHelper(const Function::Ptr& callback, const Future::Ptr& future) {
	return callback->Invoke({ future->Get() });
}

Future::Ptr Future::ContinueWith(const Function::Ptr& callback)
{
	return ScriptUtils::CallAsync({ new Function("<anonymous>", FutureContinueWithHelper), callback, this });
}

