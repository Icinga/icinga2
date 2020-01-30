/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKRESULTREADER_H
#define CHECKRESULTREADER_H

#include "base/timer.hpp"
#include "compat/checkresultreader-ti.hpp"
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

#endif /* CHECKRESULTREADER_H */
