// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
