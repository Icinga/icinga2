// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PERFDATAWRITER_H
#define PERFDATAWRITER_H

#include "base/configobject.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "icinga/service.hpp"
#include "perfdata/perfdatawriter-ti.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga perfdata writer.
 *
 * @ingroup icinga
 */
class PerfdataWriter final : public ObjectImpl<PerfdataWriter>
{
public:
	DECLARE_OBJECT(PerfdataWriter);
	DECLARE_OBJECTNAME(PerfdataWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidateHostFormatTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils) override;
	void ValidateServiceFormatTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	boost::signals2::connection m_HandleCheckResults;
	Timer::Ptr m_RotationTimer;
	std::ofstream m_ServiceOutputFile;
	std::ofstream m_HostOutputFile;
	WorkQueue m_WorkQueue{10000000, 1};

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	static Value EscapeMacroMetric(const Value& value);

	void RotationTimerHandler();
	void RotateAllFiles();
	void RotateFile(std::ofstream& output, const String& temp_path, const String& perfdata_path);

	void AssertOnWorkQueue();
	void ExceptionHandler(std::exception_ptr exp);
};

}

#endif /* PERFDATAWRITER_H */
