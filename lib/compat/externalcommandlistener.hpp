// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EXTERNALCOMMANDLISTENER_H
#define EXTERNALCOMMANDLISTENER_H

#include "compat/externalcommandlistener-ti.hpp"
#include "base/objectlock.hpp"
#include "base/wait-group.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include <thread>
#include <iostream>

namespace icinga
{

/**
 * @ingroup compat
 */
class ExternalCommandListener final : public ObjectImpl<ExternalCommandListener>
{
public:
	DECLARE_OBJECT(ExternalCommandListener);
	DECLARE_OBJECTNAME(ExternalCommandListener);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	StoppableWaitGroup::Ptr m_WaitGroup = new StoppableWaitGroup();

#ifndef _WIN32
	std::thread m_CommandThread;

	void CommandPipeThread(const String& commandPath);
#endif /* _WIN32 */
};

}

#endif /* EXTERNALCOMMANDLISTENER_H */
