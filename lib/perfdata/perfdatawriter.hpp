/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef PERFDATAWRITER_H
#define PERFDATAWRITER_H

#include "perfdata/perfdatawriter-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/timer.hpp"
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
	Timer::Ptr m_RotationTimer;
	std::ofstream m_ServiceOutputFile;
	std::ofstream m_HostOutputFile;
	std::mutex m_StreamMutex;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	static Value EscapeMacroMetric(const Value& value);

	void RotationTimerHandler();
	void RotateAllFiles();
	void RotateFile(std::ofstream& output, const String& temp_path, const String& perfdata_path);
};

}

#endif /* PERFDATAWRITER_H */
