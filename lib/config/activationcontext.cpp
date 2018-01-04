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

#include "config/activationcontext.hpp"
#include "base/exception.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::stack<ActivationContext::Ptr> > ActivationContext::m_ActivationStack;

std::stack<ActivationContext::Ptr>& ActivationContext::GetActivationStack()
{
	std::stack<ActivationContext::Ptr> *actx = m_ActivationStack.get();

	if (!actx) {
		actx = new std::stack<ActivationContext::Ptr>();
		m_ActivationStack.reset(actx);
	}

	return *actx;
}

void ActivationContext::PushContext(const ActivationContext::Ptr& context)
{
	GetActivationStack().push(context);
}

void ActivationContext::PopContext()
{
	ASSERT(!GetActivationStack().empty());
	GetActivationStack().pop();
}

ActivationContext::Ptr ActivationContext::GetCurrentContext()
{
	std::stack<ActivationContext::Ptr>& astack = GetActivationStack();

	if (astack.empty())
		BOOST_THROW_EXCEPTION(std::runtime_error("Objects may not be created outside of an activation context."));

	return astack.top();
}

ActivationScope::ActivationScope(ActivationContext::Ptr context)
	: m_Context(std::move(context))
{
	if (!m_Context)
		m_Context = new ActivationContext();

	ActivationContext::PushContext(m_Context);
}

ActivationScope::~ActivationScope()
{
	ActivationContext::PopContext();
}

ActivationContext::Ptr ActivationScope::GetContext() const
{
	return m_Context;
}

