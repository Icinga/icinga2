/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "perfdata/perfdatawriter.hpp"
#include "perfdata/perfdatawriter-ti.cpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/context.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

REGISTER_TYPE(PerfdataWriter);

REGISTER_STATSFUNCTION(PerfdataWriter, &PerfdataWriter::StatsFunc);

void PerfdataWriter::OnConfigLoaded()
{
	ObjectImpl<PerfdataWriter>::OnConfigLoaded();

	if (!GetEnableHa()) {
		Log(LogDebug, "PerfdataWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void PerfdataWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	DictionaryData nodes;

	for (const PerfdataWriter::Ptr& perfdatawriter : ConfigType::GetObjectsByType<PerfdataWriter>()) {
		nodes.emplace_back(perfdatawriter->GetName(), 1); //add more stats
	}

	status->Set("perfdatawriter", new Dictionary(std::move(nodes)));
}

void PerfdataWriter::Resume()
{
	ObjectImpl<PerfdataWriter>::Resume();

	Log(LogInformation, "PerfdataWriter")
		<< "'" << GetName() << "' resumed.";

	Checkable::OnNewCheckResult.connect([this](const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
		CheckResultHandler(checkable, cr);
	});

	m_RotationTimer = new Timer();
	m_RotationTimer->OnTimerExpired.connect([this](const Timer * const&) { RotationTimerHandler(); });
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

void PerfdataWriter::Pause()
{
	m_RotationTimer.reset();

#ifdef I2_DEBUG
	//m_HostOutputFile << "\n# Pause the feature" << "\n\n";
	//m_ServiceOutputFile << "\n# Pause the feature" << "\n\n";
#endif /* I2_DEBUG */

	/* Force a rotation closing the file stream. */
	RotateAllFiles();

	Log(LogInformation, "PerfdataWriter")
		<< "'" << GetName() << "' paused.";

	ObjectImpl<PerfdataWriter>::Pause();
}

Value PerfdataWriter::EscapeMacroMetric(const Value& value)
{
	if (value.IsObjectType<Array>())
		return Utility::Join(value, ';');
	else
		return value;
}

void PerfdataWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	CONTEXT("Writing performance data for object '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Service::Ptr service = dynamic_pointer_cast<Service>(checkable);
	Host::Ptr host;

	if (service)
		host = service->GetHost();
	else
		host = static_pointer_cast<Host>(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	if (service) {
		String line = MacroProcessor::ResolveMacros(GetServiceFormatTemplate(), resolvers, cr, nullptr, &PerfdataWriter::EscapeMacroMetric);

		{
			boost::mutex::scoped_lock lock(m_StreamMutex);

			if (!m_ServiceOutputFile.good())
				return;

			m_ServiceOutputFile << line << "\n";
		}
	} else {
		String line = MacroProcessor::ResolveMacros(GetHostFormatTemplate(), resolvers, cr, nullptr, &PerfdataWriter::EscapeMacroMetric);

		{
			boost::mutex::scoped_lock lock(m_StreamMutex);

			if (!m_HostOutputFile.good())
				return;

			m_HostOutputFile << line << "\n";
		}
	}
}

void PerfdataWriter::RotateFile(std::ofstream& output, const String& temp_path, const String& perfdata_path)
{
	Log(LogDebug, "PerfdataWriter")
		<< "Rotating perfdata files.";

	boost::mutex::scoped_lock lock(m_StreamMutex);

	if (output.good()) {
		output.close();

		if (Utility::PathExists(temp_path)) {
			String finalFile = perfdata_path + "." + Convert::ToString((long)Utility::GetTime());

			Log(LogDebug, "PerfdataWriter")
				<< "Closed output file and renaming into '" << finalFile << "'.";

			Utility::RenameFile(temp_path, finalFile);
		}
	}

	output.open(temp_path.CStr());

	if (!output.good()) {
		Log(LogWarning, "PerfdataWriter")
			<< "Could not open perfdata file '" << temp_path << "' for writing. Perfdata will be lost.";
	}
}

void PerfdataWriter::RotationTimerHandler()
{
	if (IsPaused())
		return;

	RotateAllFiles();
}

void PerfdataWriter::RotateAllFiles()
{
	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

void PerfdataWriter::ValidateHostFormatTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<PerfdataWriter>::ValidateHostFormatTemplate(lvalue, utils);

	if (!MacroProcessor::ValidateMacroString(lvalue()))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "host_format_template" }, "Closing $ not found in macro format string '" + lvalue() + "'."));
}

void PerfdataWriter::ValidateServiceFormatTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<PerfdataWriter>::ValidateServiceFormatTemplate(lvalue, utils);

	if (!MacroProcessor::ValidateMacroString(lvalue()))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "service_format_template" }, "Closing $ not found in macro format string '" + lvalue() + "'."));
}
