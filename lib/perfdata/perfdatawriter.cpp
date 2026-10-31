// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "perfdata/perfdatawriter.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/context.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"
#include "base/utility.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/service.hpp"
#include "perfdata/perfdatawriter-ti.cpp"

using namespace icinga;

REGISTER_TYPE(PerfdataWriter);

REGISTER_STATSFUNCTION(PerfdataWriter, &PerfdataWriter::StatsFunc);

void PerfdataWriter::OnConfigLoaded()
{
	ObjectImpl<PerfdataWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("PerfdataWriter, " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, "PerfdataWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void PerfdataWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const PerfdataWriter::Ptr& perfdatawriter : ConfigType::GetObjectsByType<PerfdataWriter>()) {
		size_t workQueueItems    = perfdatawriter->m_WorkQueue.GetLength();
		double workQueueItemRate = perfdatawriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		nodes.emplace_back(
			perfdatawriter->GetName(),
			new Dictionary({{"work_queue_items", workQueueItems}, {"work_queue_item_rate", workQueueItemRate}})
		);

		perfdata->Add(
			new PerfdataValue("perfdatawriter_" + perfdatawriter->GetName() + "_work_queue_items", workQueueItems)
		);
		perfdata->Add(
			new PerfdataValue("perfdatawriter_" + perfdatawriter->GetName() + "_work_queue_item_rate", workQueueItemRate)
		);
	}

	status->Set("perfdatawriter", new Dictionary(std::move(nodes)));
}

void PerfdataWriter::Resume()
{
	ObjectImpl<PerfdataWriter>::Resume();

	Log(LogInformation, "PerfdataWriter")
		<< "'" << GetName() << "' resumed.";

	/* Register exception handler for WQ tasks. */
	m_WorkQueue.SetExceptionCallback([this](std::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	/* All file operations happen on the WQ thread. Open the files before the first check result is queued. */
	m_WorkQueue.Enqueue([this]() { RotateAllFiles(); });

	m_HandleCheckResults = Checkable::OnNewCheckResult.connect([this](const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
		CheckResultHandler(checkable, cr);
	});

	m_RotationTimer = Timer::Create();
	m_RotationTimer->OnTimerExpired.connect([this](const Timer * const&) { RotationTimerHandler(); });
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();
}

void PerfdataWriter::Pause()
{
	m_HandleCheckResults.disconnect();
	m_RotationTimer->Stop(true);

#ifdef I2_DEBUG
	//m_HostOutputFile << "\n# Pause the feature" << "\n\n";
	//m_ServiceOutputFile << "\n# Pause the feature" << "\n\n";
#endif /* I2_DEBUG */

	/*
	 * Force a rotation closing the file stream. It's queued behind all pending writes,
	 * so Join() returns only after every queued check result has been written.
	 */
	m_WorkQueue.Enqueue([this]() { RotateAllFiles(); });
	m_WorkQueue.Join();

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

	CONTEXT("Writing performance data for object '" << checkable->GetName() << "'");

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

	/* Resolve the macros now, as they depend on the current state of the checkable. */
	String line = MacroProcessor::ResolveMacros(
		service ? GetServiceFormatTemplate() : GetHostFormatTemplate(), resolvers, cr, nullptr, &PerfdataWriter::EscapeMacroMetric
	);
	std::ofstream& output = service ? m_ServiceOutputFile : m_HostOutputFile;

	m_WorkQueue.Enqueue([this, &output, line = std::move(line)]() {
		AssertOnWorkQueue();

		if (!output.good()) {
			return;
		}

		output << line << "\n";
	});
}

void PerfdataWriter::RotateFile(std::ofstream& output, const String& temp_path, const String& perfdata_path)
{
	Log(LogDebug, "PerfdataWriter")
		<< "Rotating perfdata files.";

	AssertOnWorkQueue();

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

	m_WorkQueue.Enqueue([this]() { RotateAllFiles(); });
}

void PerfdataWriter::RotateAllFiles()
{
	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

/**
 * Check if method is called inside the WQ thread.
 */
void PerfdataWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

/**
 * Exception handler for the WQ.
 *
 * @param exp Exception pointer
 */
void PerfdataWriter::ExceptionHandler(std::exception_ptr exp)
{
	Log(LogCritical,
		"PerfdataWriter",
		"Exception during perfdata file operation: Verify that the perfdata directories are writable!");

	Log(LogDebug, "PerfdataWriter")
		<< "Exception during perfdata file operation: " << DiagnosticInformation(std::move(exp));
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
