/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "compat/externalcommandlistener-ti.hpp"
#include "base/objectlock.hpp"
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
#ifndef _WIN32
	std::thread m_CommandThread;

	void CommandPipeThread(const String& commandPath);
#endif /* _WIN32 */
};

}
