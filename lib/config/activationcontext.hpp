// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ACTIVATIONCONTEXT_H
#define ACTIVATIONCONTEXT_H

#include "config/i2-config.hpp"
#include "base/object.hpp"
#include <boost/thread/tss.hpp>
#include <stack>

namespace icinga
{

class ActivationContext final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ActivationContext);

	static ActivationContext::Ptr GetCurrentContext();

private:
	static void PushContext(const ActivationContext::Ptr& context);
	static void PopContext();

	static std::stack<ActivationContext::Ptr>& GetActivationStack();

	static boost::thread_specific_ptr<std::stack<ActivationContext::Ptr> > m_ActivationStack;

	friend class ActivationScope;
};

class ActivationScope
{
public:
	ActivationScope(ActivationContext::Ptr context = nullptr);
	~ActivationScope();

	ActivationContext::Ptr GetContext() const;

private:
	ActivationContext::Ptr m_Context;
};

}

#endif /* ACTIVATIONCONTEXT_H */
