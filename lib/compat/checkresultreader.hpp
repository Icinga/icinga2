/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "compat/checkresultreader-ti.hpp"
#include "base/timer.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga checkresult reader.
 *
 * @ingroup compat
 */
class CheckResultReader final : public ObjectImpl<CheckResultReader>
{
public:
	DECLARE_OBJECT(CheckResultReader);
	DECLARE_OBJECTNAME(CheckResultReader);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	Timer::Ptr m_ReadTimer;
	void ReadTimerHandler() const;
	void ProcessCheckResultFile(const String& path) const;
};

}
