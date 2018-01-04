/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "compat/statusdatawriter.hpp"
#include "compat/statusdatawriter.tcpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/cib.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/dependency.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

using namespace icinga;

REGISTER_TYPE(StatusDataWriter);

REGISTER_STATSFUNCTION(StatusDataWriter, &StatusDataWriter::StatsFunc);

void StatusDataWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const StatusDataWriter::Ptr& statusdatawriter : ConfigType::GetObjectsByType<StatusDataWriter>()) {
		nodes->Set(statusdatawriter->GetName(), 1); //add more stats
	}

	status->Set("statusdatawriter", nodes);
}

/**
 * Hint: The reason why we're using "\n" rather than std::endl is because
 * std::endl also _flushes_ the output stream which severely degrades
 * performance (see https://stackoverflow.com/questions/213907/c-stdendl-vs-n).
 */

/**
 * Starts the component.
 */
void StatusDataWriter::Start(bool runtimeCreated)
{
	ObjectImpl<StatusDataWriter>::Start(runtimeCreated);

	Log(LogInformation, "StatusDataWriter")
		<< "'" << GetName() << "' started.";

	m_ObjectsCacheOutdated = true;

	m_StatusTimer = new Timer();
	m_StatusTimer->SetInterval(GetUpdateInterval());
	m_StatusTimer->OnTimerExpired.connect(std::bind(&StatusDataWriter::StatusTimerHandler, this));
	m_StatusTimer->Start();
	m_StatusTimer->Reschedule(0);

	ConfigObject::OnVersionChanged.connect(std::bind(&StatusDataWriter::ObjectHandler, this));
	ConfigObject::OnActiveChanged.connect(std::bind(&StatusDataWriter::ObjectHandler, this));
}

/**
 * Stops the component.
 */
void StatusDataWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "StatusDataWriter")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<StatusDataWriter>::Stop(runtimeRemoved);
}

void StatusDataWriter::DumpComments(std::ostream& fp, const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	for (const Comment::Ptr& comment : checkable->GetComments()) {
		if (comment->IsExpired())
			continue;

		if (service)
			fp << "servicecomment {" << "\n"
				<< "\t" << "service_description=" << service->GetShortName() << "\n";
		else
			fp << "hostcomment {" << "\n";

		fp << "\t" "host_name=" << host->GetName() << "\n"
			"\t" "comment_id=" << comment->GetLegacyId() << "\n"
			"\t" "entry_time=" << comment->GetEntryTime() << "\n"
			"\t" "entry_type=" << comment->GetEntryType() << "\n"
			"\t" "persistent=" "1" "\n"
			"\t" "author=" << comment->GetAuthor() << "\n"
			"\t" "comment_data=" << comment->GetText() << "\n"
			"\t" "expires=" << (comment->GetExpireTime() != 0 ? 1 : 0) << "\n"
			"\t" "expire_time=" << comment->GetExpireTime() << "\n"
			"\t" "}" "\n"
			"\n";
	}
}

void StatusDataWriter::DumpTimePeriod(std::ostream& fp, const TimePeriod::Ptr& tp)
{
	fp << "define timeperiod {" "\n"
		"\t" "timeperiod_name" "\t" << tp->GetName() << "\n"
		"\t" "alias" "\t" << tp->GetName() << "\n";

	Dictionary::Ptr ranges = tp->GetRanges();

	if (ranges) {
		ObjectLock olock(ranges);
		for (const Dictionary::Pair& kv : ranges) {
			fp << "\t" << kv.first << "\t" << kv.second << "\n";
		}
	}

	fp << "\t" "}" "\n" "\n";
}

void StatusDataWriter::DumpCommand(std::ostream& fp, const Command::Ptr& command)
{
	fp << "define command {" "\n"
		"\t" "command_name\t";

	fp << CompatUtility::GetCommandName(command) << "\n";

	fp << "\t" "command_line" "\t" << CompatUtility::GetCommandLine(command);

	fp << "\n";

	DumpCustomAttributes(fp, command);

	fp << "\n" "\t" "}" "\n" "\n";
}

void StatusDataWriter::DumpDowntimes(std::ostream& fp, const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	for (const Downtime::Ptr& downtime : checkable->GetDowntimes()) {
		if (downtime->IsExpired())
			continue;

		if (service)
			fp << "servicedowntime {" << "\n"
				"\t" "service_description=" << service->GetShortName() << "\n";
		else
			fp << "hostdowntime {" "\n";

		Downtime::Ptr triggeredByObj = Downtime::GetByName(downtime->GetTriggeredBy());
		int triggeredByLegacy = 0;
		if (triggeredByObj)
			triggeredByLegacy = triggeredByObj->GetLegacyId();

		fp << "\t" << "host_name=" << host->GetName() << "\n"
			"\t" "downtime_id=" << downtime->GetLegacyId() << "\n"
			"\t" "entry_time=" << downtime->GetEntryTime() << "\n"
			"\t" "start_time=" << downtime->GetStartTime() << "\n"
			"\t" "end_time=" << downtime->GetEndTime() << "\n"
			"\t" "triggered_by=" << triggeredByLegacy << "\n"
			"\t" "fixed=" << static_cast<long>(downtime->GetFixed()) << "\n"
			"\t" "duration=" << static_cast<long>(downtime->GetDuration()) << "\n"
			"\t" "is_in_effect=" << (downtime->IsInEffect() ? 1 : 0) << "\n"
			"\t" "author=" << downtime->GetAuthor() << "\n"
			"\t" "comment=" << downtime->GetComment() << "\n"
			"\t" "trigger_time=" << downtime->GetTriggerTime() << "\n"
			"\t" "}" "\n"
			"\n";
	}
}

void StatusDataWriter::DumpHostStatus(std::ostream& fp, const Host::Ptr& host)
{
	fp << "hoststatus {" "\n" "\t" "host_name=" << host->GetName() << "\n";

	{
		ObjectLock olock(host);
		DumpCheckableStatusAttrs(fp, host);
	}

	/* ugly but cgis parse only that */
	fp << "\t" "last_time_up=" << host->GetLastStateUp() << "\n"
		"\t" "last_time_down=" << host->GetLastStateDown() << "\n"
		"\t" "last_time_unreachable=" << host->GetLastStateUnreachable() << "\n";

	fp << "\t" "}" "\n" "\n";

	DumpDowntimes(fp, host);
	DumpComments(fp, host);
}

void StatusDataWriter::DumpHostObject(std::ostream& fp, const Host::Ptr& host)
{
	String notes = host->GetNotes();
	String notes_url = host->GetNotesUrl();
	String action_url = host->GetActionUrl();
	String icon_image = host->GetIconImage();
	String icon_image_alt = host->GetIconImageAlt();
	String display_name = host->GetDisplayName();
	String address = host->GetAddress();
	String address6 = host->GetAddress6();

	fp << "define host {" "\n"
		"\t" "host_name" "\t" << host->GetName() << "\n";
	if (!display_name.IsEmpty()) {
		fp << "\t" "display_name" "\t" << host->GetDisplayName() << "\n"
			"\t" "alias" "\t" << host->GetDisplayName() << "\n";
	}
	if (!address.IsEmpty())
		fp << "\t" "address" "\t" << address << "\n";
	if (!address6.IsEmpty())
		fp << "\t" "address6" "\t" << address6 << "\n";
	if (!notes.IsEmpty())
		fp << "\t" "notes" "\t" << notes << "\n";
	if (!notes_url.IsEmpty())
		fp << "\t" "notes_url" "\t" << notes_url << "\n";
	if (!action_url.IsEmpty())
		fp << "\t" "action_url" "\t" << action_url << "\n";
	if (!icon_image.IsEmpty())
		fp << "\t" "icon_image" "\t" << icon_image << "\n";
	if (!icon_image_alt.IsEmpty())
		fp << "\t" "icon_image_alt" "\t" << icon_image_alt << "\n";

	std::set<Checkable::Ptr> parents = host->GetParents();

	if (!parents.empty()) {
		fp << "\t" "parents" "\t";
		DumpNameList(fp, parents);
		fp << "\n";
	}

	ObjectLock olock(host);

	fp << "\t" "check_interval" "\t" << CompatUtility::GetCheckableCheckInterval(host) << "\n"
		"\t" "retry_interval" "\t" << CompatUtility::GetCheckableRetryInterval(host) << "\n"
		"\t" "max_check_attempts" "\t" << host->GetMaxCheckAttempts() << "\n"
		"\t" "active_checks_enabled" "\t" << CompatUtility::GetCheckableActiveChecksEnabled(host) << "\n"
		"\t" "passive_checks_enabled" "\t" << CompatUtility::GetCheckablePassiveChecksEnabled(host) << "\n"
		"\t" "notifications_enabled" "\t" << CompatUtility::GetCheckableNotificationsEnabled(host) << "\n"
		"\t" "notification_options" "\t" << CompatUtility::GetCheckableNotificationNotificationOptions(host) << "\n"
		"\t" "notification_interval" "\t" << CompatUtility::GetCheckableNotificationNotificationInterval(host) << "\n"
		"\t" "event_handler_enabled" "\t" << CompatUtility::GetCheckableEventHandlerEnabled(host) << "\n";

	CheckCommand::Ptr checkcommand = host->GetCheckCommand();
	if (checkcommand)
		fp << "\t" "check_command" "\t" << CompatUtility::GetCommandName(checkcommand) << "!" << CompatUtility::GetCheckableCommandArgs(host) << "\n";

	EventCommand::Ptr eventcommand = host->GetEventCommand();
	if (eventcommand && host->GetEnableEventHandler())
		fp << "\t" "event_handler" "\t" << CompatUtility::GetCommandName(eventcommand) << "\n";

	fp << "\t" "check_period" "\t" << CompatUtility::GetCheckableCheckPeriod(host) << "\n";

	fp << "\t" "contacts" "\t";
	DumpNameList(fp, CompatUtility::GetCheckableNotificationUsers(host));
	fp << "\n";

	fp << "\t" "contact_groups" "\t";
	DumpNameList(fp, CompatUtility::GetCheckableNotificationUserGroups(host));
	fp << "\n";

	fp << "\t" << "initial_state" "\t" "o" "\n"
		"\t" "low_flap_threshold" "\t" << host->GetFlappingThresholdLow() << "\n"
		"\t" "high_flap_threshold" "\t" << host->GetFlappingThresholdHigh() << "\n"
		"\t" "process_perf_data" "\t" << CompatUtility::GetCheckableProcessPerformanceData(host) << "\n"
		"\t" "check_freshness" "\t" "1" "\n";

	fp << "\t" "host_groups" "\t";
	bool first = true;

	Array::Ptr groups = host->GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg) {
				if (!first)
					fp << ",";
				else
					first = false;

				fp << hg->GetName();
			}
		}
	}

	fp << "\n";

	DumpCustomAttributes(fp, host);

	fp << "\t" "}" "\n" "\n";
}

void StatusDataWriter::DumpCheckableStatusAttrs(std::ostream& fp, const Checkable::Ptr& checkable)
{
	CheckResult::Ptr cr = checkable->GetLastCheckResult();

	EventCommand::Ptr eventcommand = checkable->GetEventCommand();
	CheckCommand::Ptr checkcommand = checkable->GetCheckCommand();

	fp << "\t" << "check_command=" << CompatUtility::GetCommandName(checkcommand) << "!" << CompatUtility::GetCheckableCommandArgs(checkable) << "\n"
		"\t" "event_handler=" << CompatUtility::GetCommandName(eventcommand) << "\n"
		"\t" "check_period=" << CompatUtility::GetCheckableCheckPeriod(checkable) << "\n"
		"\t" "check_interval=" << CompatUtility::GetCheckableCheckInterval(checkable) << "\n"
		"\t" "retry_interval=" << CompatUtility::GetCheckableRetryInterval(checkable) << "\n"
		"\t" "has_been_checked=" << CompatUtility::GetCheckableHasBeenChecked(checkable) << "\n"
		"\t" "should_be_scheduled=" << checkable->GetEnableActiveChecks() << "\n"
		"\t" "event_handler_enabled=" << CompatUtility::GetCheckableEventHandlerEnabled(checkable) << "\n";

	if (cr) {
		fp << "\t" << "check_execution_time=" << Convert::ToString(cr->CalculateExecutionTime()) << "\n"
			"\t" "check_latency=" << Convert::ToString(cr->CalculateLatency()) << "\n";
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (service) {
		fp << "\t" "current_state=" << service->GetState() << "\n"
			"\t" "last_hard_state=" << service->GetLastHardState() << "\n"
			"\t" "last_time_ok=" << static_cast<int>(service->GetLastStateOK()) << "\n"
			"\t" "last_time_warn=" << static_cast<int>(service->GetLastStateWarning()) << "\n"
			"\t" "last_time_critical=" << static_cast<int>(service->GetLastStateCritical()) << "\n"
			"\t" "last_time_unknown=" << static_cast<int>(service->GetLastStateUnknown()) << "\n";
	} else {
		fp << "\t" "current_state=" << CompatUtility::GetHostCurrentState(host) << "\n"
			"\t" "last_hard_state=" << host->GetLastHardState() << "\n"
			"\t" "last_time_up=" << static_cast<int>(host->GetLastStateUp()) << "\n"
			"\t" "last_time_down=" << static_cast<int>(host->GetLastStateDown()) << "\n";
	}

	fp << "\t" "state_type=" << checkable->GetStateType() << "\n"
		"\t" "plugin_output=" << CompatUtility::GetCheckResultOutput(cr) << "\n"
		"\t" "long_plugin_output=" << CompatUtility::GetCheckResultLongOutput(cr) << "\n"
		"\t" "performance_data=" << CompatUtility::GetCheckResultPerfdata(cr) << "\n";

	if (cr) {
		fp << "\t" << "check_source=" << cr->GetCheckSource() << "\n"
		"\t" "last_check=" << static_cast<long>(cr->GetScheduleEnd()) << "\n";
	}

	fp << "\t" << "next_check=" << static_cast<long>(checkable->GetNextCheck()) << "\n"
		"\t" "current_attempt=" << checkable->GetCheckAttempt() << "\n"
		"\t" "max_attempts=" << checkable->GetMaxCheckAttempts() << "\n"
		"\t" "last_state_change=" << static_cast<long>(checkable->GetLastStateChange()) << "\n"
		"\t" "last_hard_state_change=" << static_cast<long>(checkable->GetLastHardStateChange()) << "\n"
		"\t" "last_update=" << static_cast<long>(time(nullptr)) << "\n"
		"\t" "notifications_enabled=" << CompatUtility::GetCheckableNotificationsEnabled(checkable) << "\n"
		"\t" "active_checks_enabled=" << CompatUtility::GetCheckableActiveChecksEnabled(checkable) << "\n"
		"\t" "passive_checks_enabled=" << CompatUtility::GetCheckablePassiveChecksEnabled(checkable) << "\n"
		"\t" "flap_detection_enabled=" << CompatUtility::GetCheckableFlapDetectionEnabled(checkable) << "\n"
		"\t" "is_flapping=" << CompatUtility::GetCheckableIsFlapping(checkable) << "\n"
		"\t" "percent_state_change=" << CompatUtility::GetCheckablePercentStateChange(checkable) << "\n"
		"\t" "problem_has_been_acknowledged=" << CompatUtility::GetCheckableProblemHasBeenAcknowledged(checkable) << "\n"
		"\t" "acknowledgement_type=" << CompatUtility::GetCheckableAcknowledgementType(checkable) << "\n"
		"\t" "acknowledgement_end_time=" << checkable->GetAcknowledgementExpiry() << "\n"
		"\t" "scheduled_downtime_depth=" << checkable->GetDowntimeDepth() << "\n"
		"\t" "last_notification=" << CompatUtility::GetCheckableNotificationLastNotification(checkable) << "\n"
		"\t" "next_notification=" << CompatUtility::GetCheckableNotificationNextNotification(checkable) << "\n"
		"\t" "current_notification_number=" << CompatUtility::GetCheckableNotificationNotificationNumber(checkable) << "\n"
		"\t" "is_reachable=" << CompatUtility::GetCheckableIsReachable(checkable) << "\n";
}

void StatusDataWriter::DumpServiceStatus(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	fp << "servicestatus {" "\n"
		"\t" "host_name=" << host->GetName() << "\n"
		"\t" "service_description=" << service->GetShortName() << "\n";

	{
		ObjectLock olock(service);
		DumpCheckableStatusAttrs(fp, service);
	}

	fp << "\t" "}" "\n" "\n";

	DumpDowntimes(fp, service);
	DumpComments(fp, service);
}

void StatusDataWriter::DumpServiceObject(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	{
		ObjectLock olock(service);

		fp << "define service {" "\n"
			"\t" "host_name" "\t" << host->GetName() << "\n"
			"\t" "service_description" "\t" << service->GetShortName() << "\n"
			"\t" "display_name" "\t" << service->GetDisplayName() << "\n"
			"\t" "check_period" "\t" << CompatUtility::GetCheckableCheckPeriod(service) << "\n"
			"\t" "check_interval" "\t" << CompatUtility::GetCheckableCheckInterval(service) << "\n"
			"\t" "retry_interval" "\t" << CompatUtility::GetCheckableRetryInterval(service) << "\n"
			"\t" "max_check_attempts" "\t" << service->GetMaxCheckAttempts() << "\n"
			"\t" "active_checks_enabled" "\t" << CompatUtility::GetCheckableActiveChecksEnabled(service) << "\n"
			"\t" "passive_checks_enabled" "\t" << CompatUtility::GetCheckablePassiveChecksEnabled(service) << "\n"
			"\t" "flap_detection_enabled" "\t" << CompatUtility::GetCheckableFlapDetectionEnabled(service) << "\n"
			"\t" "is_volatile" "\t" << CompatUtility::GetCheckableIsVolatile(service) << "\n"
			"\t" "notifications_enabled" "\t" << CompatUtility::GetCheckableNotificationsEnabled(service) << "\n"
			"\t" "notification_options" "\t" << CompatUtility::GetCheckableNotificationNotificationOptions(service) << "\n"
			"\t" "notification_interval" "\t" << CompatUtility::GetCheckableNotificationNotificationInterval(service) << "\n"
			"\t" "notification_period" "\t" << "" << "\n"
			"\t" "event_handler_enabled" "\t" << CompatUtility::GetCheckableEventHandlerEnabled(service) << "\n";

		CheckCommand::Ptr checkcommand = service->GetCheckCommand();
		if (checkcommand)
			fp << "\t" "check_command" "\t" << CompatUtility::GetCommandName(checkcommand) << "!" << CompatUtility::GetCheckableCommandArgs(service)<< "\n";

		EventCommand::Ptr eventcommand = service->GetEventCommand();
		if (eventcommand && service->GetEnableEventHandler())
			fp << "\t" "event_handler" "\t" << CompatUtility::GetCommandName(eventcommand) << "\n";

		fp << "\t" "contacts" "\t";
		DumpNameList(fp, CompatUtility::GetCheckableNotificationUsers(service));
		fp << "\n";

		fp << "\t" "contact_groups" "\t";
		DumpNameList(fp, CompatUtility::GetCheckableNotificationUserGroups(service));
		fp << "\n";

		String notes = service->GetNotes();
		String notes_url = service->GetNotesUrl();
		String action_url = service->GetActionUrl();
		String icon_image = service->GetIconImage();
		String icon_image_alt = service->GetIconImageAlt();

		fp << "\t" "initial_state" "\t" "o" "\n"
			"\t" "low_flap_threshold" "\t" << service->GetFlappingThresholdLow() << "\n"
			"\t" "high_flap_threshold" "\t" << service->GetFlappingThresholdHigh() << "\n"
			"\t" "process_perf_data" "\t" << CompatUtility::GetCheckableProcessPerformanceData(service) << "\n"
			"\t" "check_freshness" << "\t" "1" "\n";
		if (!notes.IsEmpty())
			fp << "\t" "notes" "\t" << notes << "\n";
		if (!notes_url.IsEmpty())
			fp << "\t" "notes_url" "\t" << notes_url << "\n";
		if (!action_url.IsEmpty())
			fp << "\t" "action_url" "\t" << action_url << "\n";
		if (!icon_image.IsEmpty())
			fp << "\t" "icon_image" "\t" << icon_image << "\n";
		if (!icon_image_alt.IsEmpty())
			fp << "\t" "icon_image_alt" "\t" << icon_image_alt << "\n";
	}

	fp << "\t" "service_groups" "\t";
	bool first = true;

	Array::Ptr groups = service->GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
			ServiceGroup::Ptr sg = ServiceGroup::GetByName(name);

			if (sg) {
				if (!first)
					fp << ",";
				else
					first = false;

				fp << sg->GetName();
			}
		}
	}

	fp << "\n";

	DumpCustomAttributes(fp, service);

	fp << "\t" "}" "\n" "\n";
}

void StatusDataWriter::DumpCustomAttributes(std::ostream& fp, const CustomVarObject::Ptr& object)
{
	Dictionary::Ptr vars = object->GetVars();

	if (!vars)
		return;

	bool is_json = false;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		if (kv.first.IsEmpty())
			continue;

		Value value;

		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>()) {
			value = JsonEncode(kv.second);
			is_json = true;
		} else
			value = CompatUtility::EscapeString(kv.second);

		fp << "\t" "_" << kv.first << "\t" << value << "\n";
	}

	if (is_json)
		fp << "\t" "_is_json" "\t" "1" "\n";
}

void StatusDataWriter::UpdateObjectsCache()
{
	CONTEXT("Writing objects.cache file");

	String objectsPath = GetObjectsPath();

	std::fstream objectfp;
	String tempObjectsPath = Utility::CreateTempFile(objectsPath + ".XXXXXX", 0644, objectfp);

	objectfp << std::fixed;

	objectfp << "# Icinga objects cache file" "\n"
			"# This file is auto-generated. Do not modify this file." "\n"
			"\n";

	for (const Host::Ptr& host : ConfigType::GetObjectsByType<Host>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpHostObject(tempobjectfp, host);
		objectfp << tempobjectfp.str();

		for (const Service::Ptr& service : host->GetServices()) {
			std::ostringstream tempobjectfp;
			tempobjectfp << std::fixed;
			DumpServiceObject(tempobjectfp, service);
			objectfp << tempobjectfp.str();
		}
	}

	for (const HostGroup::Ptr& hg : ConfigType::GetObjectsByType<HostGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		String display_name = hg->GetDisplayName();
		String notes = hg->GetNotes();
		String notes_url = hg->GetNotesUrl();
		String action_url = hg->GetActionUrl();

		tempobjectfp << "define hostgroup {" "\n"
				"\t" "hostgroup_name" "\t" << hg->GetName() << "\n";

		if (!display_name.IsEmpty())
			tempobjectfp << "\t" "alias" "\t" << display_name << "\n";
		if (!notes.IsEmpty())
			tempobjectfp << "\t" "notes" "\t" << notes << "\n";
		if (!notes_url.IsEmpty())
			tempobjectfp << "\t" "notes_url" "\t" << notes_url << "\n";
		if (!action_url.IsEmpty())
			tempobjectfp << "\t" "action_url" "\t" << action_url << "\n";

		DumpCustomAttributes(tempobjectfp, hg);

		tempobjectfp << "\t" "members" "\t";
		DumpNameList(tempobjectfp, hg->GetMembers());
		tempobjectfp << "\n" "\t" "}" "\n";

		objectfp << tempobjectfp.str();
	}

	for (const ServiceGroup::Ptr& sg : ConfigType::GetObjectsByType<ServiceGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		String display_name = sg->GetDisplayName();
		String notes = sg->GetNotes();
		String notes_url = sg->GetNotesUrl();
		String action_url = sg->GetActionUrl();

		tempobjectfp << "define servicegroup {" "\n"
			"\t" "servicegroup_name" "\t" << sg->GetName() << "\n";

		if (!display_name.IsEmpty())
			tempobjectfp << "\t" "alias" "\t" << display_name << "\n";
		if (!notes.IsEmpty())
			tempobjectfp << "\t" "notes" "\t" << notes << "\n";
		if (!notes_url.IsEmpty())
			tempobjectfp << "\t" "notes_url" "\t" << notes_url << "\n";
		if (!action_url.IsEmpty())
			tempobjectfp << "\t" "action_url" "\t" << action_url << "\n";

		DumpCustomAttributes(tempobjectfp, sg);

		tempobjectfp << "\t" "members" "\t";

		std::vector<String> sglist;
		for (const Service::Ptr& service : sg->GetMembers()) {
			Host::Ptr host = service->GetHost();

			sglist.emplace_back(host->GetName());
			sglist.emplace_back(service->GetShortName());
		}

		DumpStringList(tempobjectfp, sglist);

		tempobjectfp << "\n" "}" "\n";

		objectfp << tempobjectfp.str();
	}

	for (const User::Ptr& user : ConfigType::GetObjectsByType<User>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		String email = user->GetEmail();
		String pager = user->GetPager();
		String alias = user->GetDisplayName();

		tempobjectfp << "define contact {" "\n"
				"\t" "contact_name" "\t" << user->GetName() << "\n";

		if (!alias.IsEmpty())
			tempobjectfp << "\t" "alias" "\t" << alias << "\n";
		if (!email.IsEmpty())
			tempobjectfp << "\t" "email" "\t" << email << "\n";
		if (!pager.IsEmpty())
			tempobjectfp << "\t" "pager" "\t" << pager << "\n";

		tempobjectfp << "\t" "service_notification_options" "\t" "w,u,c,r,f,s" "\n"
			"\t" "host_notification_options""\t" "d,u,r,f,s" "\n"
			"\t" "host_notifications_enabled" "\t" "1" "\n"
			"\t" "service_notifications_enabled" "\t" "1" "\n"
			"\t" "}" "\n"
			"\n";

		objectfp << tempobjectfp.str();
	}

	for (const UserGroup::Ptr& ug : ConfigType::GetObjectsByType<UserGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define contactgroup {" "\n"
				"\t" "contactgroup_name" "\t" << ug->GetName() << "\n"
				"\t" "alias" "\t" << ug->GetDisplayName() << "\n";

		tempobjectfp << "\t" "members" "\t";
		DumpNameList(tempobjectfp, ug->GetMembers());
		tempobjectfp << "\n"
				"\t" "}" "\n";

		objectfp << tempobjectfp.str();
	}

	for (const Command::Ptr& command : ConfigType::GetObjectsByType<CheckCommand>()) {
		DumpCommand(objectfp, command);
	}

	for (const Command::Ptr& command : ConfigType::GetObjectsByType<NotificationCommand>()) {
		DumpCommand(objectfp, command);
	}

	for (const Command::Ptr& command : ConfigType::GetObjectsByType<EventCommand>()) {
		DumpCommand(objectfp, command);
	}

	for (const TimePeriod::Ptr& tp : ConfigType::GetObjectsByType<TimePeriod>()) {
		DumpTimePeriod(objectfp, tp);
	}

	for (const Dependency::Ptr& dep : ConfigType::GetObjectsByType<Dependency>()) {
		Checkable::Ptr parent = dep->GetParent();

		if (!parent) {
			Log(LogDebug, "StatusDataWriter")
				<< "Missing parent for dependency '" << dep->GetName() << "'.";
			continue;
		}

		Host::Ptr parent_host;
		Service::Ptr parent_service;
		tie(parent_host, parent_service) = GetHostService(parent);

		Checkable::Ptr child = dep->GetChild();

		if (!child) {
			Log(LogDebug, "StatusDataWriter")
				<< "Missing child for dependency '" << dep->GetName() << "'.";
			continue;
		}

		Host::Ptr child_host;
		Service::Ptr child_service;
		tie(child_host, child_service) = GetHostService(child);

		int state_filter = dep->GetStateFilter();
		std::vector<String> failure_criteria;
		if (state_filter & StateFilterOK || state_filter & StateFilterUp)
			failure_criteria.emplace_back("o");
		if (state_filter & StateFilterWarning)
			failure_criteria.emplace_back("w");
		if (state_filter & StateFilterCritical)
			failure_criteria.emplace_back("c");
		if (state_filter & StateFilterUnknown)
			failure_criteria.emplace_back("u");
		if (state_filter & StateFilterDown)
			failure_criteria.emplace_back("d");

		String criteria = boost::algorithm::join(failure_criteria, ",");

		/* Icinga 1.x only allows host->host, service->service dependencies */
		if (!child_service && !parent_service) {
			objectfp << "define hostdependency {" "\n"
					"\t" "dependent_host_name" "\t" << child_host->GetName() << "\n"
					"\t" "host_name" "\t" << parent_host->GetName() << "\n"
					"\t" "execution_failure_criteria" "\t" << criteria << "\n"
					"\t" "notification_failure_criteria" "\t" << criteria << "\n"
					"\t" "}" "\n"
					"\n";
		} else if (child_service && parent_service){

			objectfp << "define servicedependency {" "\n"
					"\t" "dependent_host_name" "\t" << child_service->GetHost()->GetName() << "\n"
					"\t" "dependent_service_description" "\t" << child_service->GetShortName() << "\n"
					"\t" "host_name" "\t" << parent_service->GetHost()->GetName() << "\n"
					"\t" "service_description" "\t" << parent_service->GetShortName() << "\n"
					"\t" "execution_failure_criteria" "\t" << criteria << "\n"
					"\t" "notification_failure_criteria" "\t" << criteria << "\n"
					"\t" "}" "\n"
					"\n";
		}
	}

	objectfp.close();

#ifdef _WIN32
	_unlink(objectsPath.CStr());
#endif /* _WIN32 */

	if (rename(tempObjectsPath.CStr(), objectsPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempObjectsPath));
	}
}

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void StatusDataWriter::StatusTimerHandler()
{
	if (m_ObjectsCacheOutdated) {
		UpdateObjectsCache();
		m_ObjectsCacheOutdated = false;
	}

	double start = Utility::GetTime();

	String statusPath = GetStatusPath();

	std::fstream statusfp;
	String tempStatusPath = Utility::CreateTempFile(statusPath + ".XXXXXX", 0644, statusfp);

	statusfp << std::fixed;

	statusfp << "# Icinga status file" "\n"
			"# This file is auto-generated. Do not modify this file." "\n"
			"\n";

	statusfp << "info {" "\n"
			"\t" "created=" << Utility::GetTime() << "\n"
			"\t" "version=" << Application::GetAppVersion() << "\n"
			"\t" "}" "\n"
			"\n";

	statusfp << "programstatus {" "\n"
			"\t" "icinga_pid=" << Utility::GetPid() << "\n"
			"\t" "daemon_mode=1" "\n"
			"\t" "program_start=" << static_cast<long>(Application::GetStartTime()) << "\n"
			"\t" "active_host_checks_enabled=" << (IcingaApplication::GetInstance()->GetEnableHostChecks() ? 1 : 0) << "\n"
			"\t" "passive_host_checks_enabled=1" "\n"
			"\t" "active_service_checks_enabled=" << (IcingaApplication::GetInstance()->GetEnableServiceChecks() ? 1 : 0) << "\n"
			"\t" "passive_service_checks_enabled=1" "\n"
			"\t" "check_service_freshness=1" "\n"
			"\t" "check_host_freshness=1" "\n"
			"\t" "enable_notifications=" << (IcingaApplication::GetInstance()->GetEnableNotifications() ? 1 : 0) << "\n"
			"\t" "enable_event_handlers=" << (IcingaApplication::GetInstance()->GetEnableEventHandlers() ? 1 : 0) << "\n"
			"\t" "enable_flap_detection=" << (IcingaApplication::GetInstance()->GetEnableFlapping() ? 1 : 0) << "\n"
			"\t" "enable_failure_prediction=0" "\n"
			"\t" "process_performance_data=" << (IcingaApplication::GetInstance()->GetEnablePerfdata() ? 1 : 0) << "\n"
			"\t" "active_scheduled_host_check_stats=" << CIB::GetActiveHostChecksStatistics(60) << "," << CIB::GetActiveHostChecksStatistics(5 * 60) << "," << CIB::GetActiveHostChecksStatistics(15 * 60) << "\n"
			"\t" "passive_host_check_stats=" << CIB::GetPassiveHostChecksStatistics(60) << "," << CIB::GetPassiveHostChecksStatistics(5 * 60) << "," << CIB::GetPassiveHostChecksStatistics(15 * 60) << "\n"
			"\t" "active_scheduled_service_check_stats=" << CIB::GetActiveServiceChecksStatistics(60) << "," << CIB::GetActiveServiceChecksStatistics(5 * 60) << "," << CIB::GetActiveServiceChecksStatistics(15 * 60) << "\n"
			"\t" "passive_service_check_stats=" << CIB::GetPassiveServiceChecksStatistics(60) << "," << CIB::GetPassiveServiceChecksStatistics(5 * 60) << "," << CIB::GetPassiveServiceChecksStatistics(15 * 60) << "\n"
			"\t" "next_downtime_id=" << Downtime::GetNextDowntimeID() << "\n"
			"\t" "next_comment_id=" << Comment::GetNextCommentID() << "\n";

	statusfp << "\t" "}" "\n"
			"\n";

	for (const Host::Ptr& host : ConfigType::GetObjectsByType<Host>()) {
		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpHostStatus(tempstatusfp, host);
		statusfp << tempstatusfp.str();

		for (const Service::Ptr& service : host->GetServices()) {
			std::ostringstream tempstatusfp;
			tempstatusfp << std::fixed;
			DumpServiceStatus(tempstatusfp, service);
			statusfp << tempstatusfp.str();
		}
	}

	statusfp.close();

#ifdef _WIN32
	_unlink(statusPath.CStr());
#endif /* _WIN32 */

	if (rename(tempStatusPath.CStr(), statusPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempStatusPath));
	}

	Log(LogNotice, "StatusDataWriter")
		<< "Writing status.dat file took " << Utility::FormatDuration(Utility::GetTime() - start);
}

void StatusDataWriter::ObjectHandler()
{
	m_ObjectsCacheOutdated = true;
}
