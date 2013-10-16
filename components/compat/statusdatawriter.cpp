/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "compat/statusdatawriter.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "icinga/hostgroup.h"
#include "icinga/servicegroup.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/timeperiod.h"
#include "icinga/notificationcommand.h"
#include "icinga/compatutility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/application.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

using namespace icinga;

REGISTER_TYPE(StatusDataWriter);

/**
 * Hint: The reason why we're using "\n" rather than std::endl is because
 * std::endl also _flushes_ the output stream which severely degrades
 * performance (see http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt11ch25s02.html).
 */

/**
 * Starts the component.
 */
void StatusDataWriter::Start(void)
{
	DynamicObject::Start();

	m_StatusTimer = boost::make_shared<Timer>();
	m_StatusTimer->SetInterval(15);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&StatusDataWriter::StatusTimerHandler, this));
	m_StatusTimer->Start();
	m_StatusTimer->Reschedule(0);
}

/**
 * Retrieves the status.dat path.
 *
 * @returns statuspath from config, or static default
 */
String StatusDataWriter::GetStatusPath(void) const
{
	if (m_StatusPath.IsEmpty())
		return Application::GetLocalStateDir() + "/cache/icinga2/status.dat";
	else
		return m_StatusPath;
}

/**
 * Retrieves the objects.cache path.
 *
 * @returns objectspath from config, or static default
 */
String StatusDataWriter::GetObjectsPath(void) const
{
	if (m_ObjectsPath.IsEmpty())
		return Application::GetLocalStateDir() + "/cache/icinga2/objects.cache";
	else
		return m_ObjectsPath;
}

void StatusDataWriter::DumpComments(std::ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Service::Ptr service;
	Dictionary::Ptr comments = owner->GetComments();

	if (!comments)
		return;

	Host::Ptr host = owner->GetHost();

	if (!host)
		return;

	ObjectLock olock(comments);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {
		if (Service::IsCommentExpired(comment))
			continue;

		if (type == CompatTypeHost)
			fp << "hostcomment {" << "\n";
		else
			fp << "servicecomment {" << "\n"
			   << "\t" << "service_description=" << owner->GetShortName() << "\n";

		fp << "\t" << "host_name=" << host->GetName() << "\n"
		   << "\t" << "comment_id=" << static_cast<String>(comment->Get("legacy_id")) << "\n"
		   << "\t" << "entry_time=" << static_cast<double>(comment->Get("entry_time")) << "\n"
		   << "\t" << "entry_type=" << static_cast<long>(comment->Get("entry_type")) << "\n"
		   << "\t" << "persistent=" << 1 << "\n"
		   << "\t" << "author=" << static_cast<String>(comment->Get("author")) << "\n"
		   << "\t" << "comment_data=" << static_cast<String>(comment->Get("text")) << "\n"
		   << "\t" << "expires=" << (static_cast<double>(comment->Get("expire_time")) != 0 ? 1 : 0) << "\n"
		   << "\t" << "expire_time=" << static_cast<double>(comment->Get("expire_time")) << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void StatusDataWriter::DumpTimePeriod(std::ostream& fp, const TimePeriod::Ptr& tp)
{
	fp << "define timeperiod {" << "\n"
	   << "\t" << "timeperiod_name" << "\t" << tp->GetName() << "\n"
	   << "\t" << "alias" << "\t" << tp->GetName() << "\n";

	Dictionary::Ptr ranges = tp->GetRanges();

	if (ranges) {
		ObjectLock olock(ranges);
		String key;
		Value value;
		BOOST_FOREACH(boost::tie(key, value), ranges) {
			fp << "\t" << key << "\t" << Convert::ToString(value) << "\n";
		}
	}

	fp << "\t" << "}" << "\n"
	   << "\n";
}

void StatusDataWriter::DumpCommand(std::ostream& fp, const Command::Ptr& command)
{
	fp << "define command {" << "\n"
	   << "\t" << "command_name\t";


	if (command->GetType() == DynamicType::GetByName("CheckCommand"))
		fp << "check_";
	else if (command->GetType() == DynamicType::GetByName("NotificationCommand"))
		fp << "notification_";
	else if (command->GetType() == DynamicType::GetByName("EventCommand"))
		fp << "event_";

	fp << command->GetName() << "\n";

	fp << "\t" << "command_line\t";

	Value commandLine = command->GetCommandLine();

	if (commandLine.IsObjectType<Array>()) {
		Array::Ptr args = commandLine;

		ObjectLock olock(args);
		String arg;
		BOOST_FOREACH(arg, args) {
			// This is obviously incorrect for non-trivial cases.
			fp << " \"" << CompatUtility::EscapeString(arg) << "\"";
		}
	} else if (!commandLine.IsEmpty()) {
		fp << CompatUtility::EscapeString(Convert::ToString(commandLine));
	} else {
		fp << "<internal>";
	}

	fp << "\n";

	fp << "\t" << "}" << "\n"
	   << "\n";

}

void StatusDataWriter::DumpDowntimes(std::ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Host::Ptr host = owner->GetHost();

	if (!host)
		return;

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	if (!downtimes)
		return;

	ObjectLock olock(downtimes);

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {
		if (Service::IsDowntimeExpired(downtime))
			continue;

		if (type == CompatTypeHost)
			fp << "hostdowntime {" << "\n";
		else
			fp << "servicedowntime {" << "\n"
			   << "\t" << "service_description=" << owner->GetShortName() << "\n";

		Dictionary::Ptr triggeredByObj = Service::GetDowntimeByID(downtime->Get("triggered_by"));
		int triggeredByLegacy = 0;
		if (triggeredByObj)
			triggeredByLegacy = triggeredByObj->Get("legacy_id");

		fp << "\t" << "host_name=" << host->GetName() << "\n"
		   << "\t" << "downtime_id=" << static_cast<String>(downtime->Get("legacy_id")) << "\n"
		   << "\t" << "entry_time=" << static_cast<double>(downtime->Get("entry_time")) << "\n"
		   << "\t" << "start_time=" << static_cast<double>(downtime->Get("start_time")) << "\n"
		   << "\t" << "end_time=" << static_cast<double>(downtime->Get("end_time")) << "\n"
		   << "\t" << "triggered_by=" << triggeredByLegacy << "\n"
		   << "\t" << "fixed=" << static_cast<long>(downtime->Get("fixed")) << "\n"
		   << "\t" << "duration=" << static_cast<long>(downtime->Get("duration")) << "\n"
		   << "\t" << "is_in_effect=" << (Service::IsDowntimeActive(downtime) ? 1 : 0) << "\n"
		   << "\t" << "author=" << static_cast<String>(downtime->Get("author")) << "\n"
		   << "\t" << "comment=" << static_cast<String>(downtime->Get("comment")) << "\n"
		   << "\t" << "trigger_time=" << static_cast<double>(downtime->Get("trigger_time")) << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void StatusDataWriter::DumpHostStatus(std::ostream& fp, const Host::Ptr& host)
{
	fp << "hoststatus {" << "\n"
	   << "\t" << "host_name=" << host->GetName() << "\n";

	Service::Ptr hc = host->GetCheckService();
	ObjectLock olock(hc);

	if (hc)
		DumpServiceStatusAttrs(fp, hc, CompatTypeHost);

	/* ugly but cgis parse only that */
	fp << "\t" << "last_time_up=" << host->GetLastStateUp() << "\n"
	   << "\t" << "last_time_down=" << host->GetLastStateDown() << "\n"
	   << "\t" << "last_time_unreachable=" << host->GetLastStateUnreachable() << "\n";

	fp << "\t" << "}" << "\n"
	   << "\n";

	if (hc) {
		DumpDowntimes(fp, hc, CompatTypeHost);
		DumpComments(fp, hc, CompatTypeHost);
	}
}

void StatusDataWriter::DumpHostObject(std::ostream& fp, const Host::Ptr& host)
{
	fp << "define host {" << "\n"
	   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
	   << "\t" << "display_name" << "\t" << host->GetDisplayName() << "\n"
	   << "\t" << "alias" << "\t" << host->GetDisplayName() << "\n";

	Dictionary::Ptr macros = host->GetMacros();

	if (macros) {
		fp << "\t" << "address" << "\t" << macros->Get("address") << "\n"
		   << "\t" << "address6" << "\t" << macros->Get("address6") << "\n";
	}

	std::set<Host::Ptr> parents = host->GetParentHosts();

	if (!parents.empty()) {
		fp << "\t" << "parents" << "\t";
		DumpNameList(fp, parents);
		fp << "\n";
	}

	Service::Ptr hc = host->GetCheckService();
	if (hc) {
		ObjectLock olock(hc);

		fp << "\t" << "check_interval" << "\t" << hc->GetCheckInterval() / 60.0 << "\n"
		   << "\t" << "retry_interval" << "\t" << hc->GetRetryInterval() / 60.0 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << hc->GetMaxCheckAttempts() << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << (hc->GetEnableActiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << (hc->GetEnablePassiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "notifications_enabled" << "\t" << (hc->GetEnableNotifications() ? 1 : 0) << "\n"
		   << "\t" << "notification_options" << "\t" << "d,u,r" << "\n"
		   << "\t" << "notification_interval" << "\t" << 1 << "\n"
		   << "\t" << "event_handler_enabled" << "\t" << (hc->GetEnableEventHandler() ? 1 : 0) << "\n";

		CheckCommand::Ptr checkcommand = hc->GetCheckCommand();
		if (checkcommand)
			fp << "\t" << "check_command" << "\t" << "check_" << checkcommand->GetName() << "\n";

		EventCommand::Ptr eventcommand = hc->GetEventCommand();
		if (eventcommand)
			fp << "\t" << "event_handler" << "\t" << "event_" << eventcommand->GetName() << "\n";

		TimePeriod::Ptr check_period = hc->GetCheckPeriod();
		if (check_period)
			fp << "\t" << "check_period" << "\t" << check_period->GetName() << "\n";

		fp << "\t" << "contacts" << "\t";
		DumpNameList(fp, CompatUtility::GetServiceNotificationUsers(hc));
		fp << "\n";

		fp << "\t" << "contact_groups" << "\t";
		DumpNameList(fp, CompatUtility::GetServiceNotificationUserGroups(hc));
		fp << "\n";

		fp << "\t" << "initial_state" << "\t" << "o" << "\n"
		   << "\t" << "low_flap_threshold" << "\t" << hc->GetFlappingThreshold() << "\n"
		   << "\t" << "high_flap_threshold" << "\t" << hc->GetFlappingThreshold() << "\n"
		   << "\t" << "process_perf_data" << "\t" << 1 << "\n"
		   << "\t" << "check_freshness" << "\t" << 1 << "\n";

	} else {
		fp << "\t" << "check_interval" << "\t" << 60 << "\n"
		   << "\t" << "retry_interval" << "\t" << 60 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << 0 << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << 0 << "\n"
		   << "\t" << "notifications_enabled" << "\t" << 0 << "\n";

	}

	fp << "\t" << "host_groups" << "\t";
	bool first = true;

        Array::Ptr groups = host->GetGroups();

        if (groups) {
                ObjectLock olock(groups);

                BOOST_FOREACH(const String& name, groups) {
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

	fp << "\t" << "}" << "\n"
	   << "\n";
}

void StatusDataWriter::DumpServiceStatusAttrs(std::ostream& fp, const Service::Ptr& service, CompatObjectType type)
{
	Dictionary::Ptr attrs = CompatUtility::GetServiceStatusAttributes(service, type);

	fp << "\t" << "check_command=" << attrs->Get("check_command") << "\n"
	   << "\t" << "event_handler=" << attrs->Get("event_handler") << "\n"
	   << "\t" << "check_period=" << attrs->Get("check_period") << "\n"
	   << "\t" << "check_interval=" << static_cast<double>(attrs->Get("check_interval")) << "\n"
	   << "\t" << "retry_interval=" << static_cast<double>(attrs->Get("retry_interval")) << "\n"
	   << "\t" << "has_been_checked=" << attrs->Get("has_been_checked") << "\n"
	   << "\t" << "should_be_scheduled=" << attrs->Get("should_be_scheduled") << "\n"
	   << "\t" << "check_execution_time=" << static_cast<double>(attrs->Get("check_execution_time")) << "\n"
	   << "\t" << "check_latency=" << static_cast<double>(attrs->Get("check_latency")) << "\n"
	   << "\t" << "current_state=" << attrs->Get("current_state") << "\n"
	   << "\t" << "state_type=" << attrs->Get("state_type") << "\n"
	   << "\t" << "plugin_output=" << attrs->Get("plugin_output") << "\n"
	   << "\t" << "long_plugin_output=" << attrs->Get("long_plugin_output") << "\n"
	   << "\t" << "performance_data=" << attrs->Get("performance_data") << "\n"
	   << "\t" << "check_source=" << attrs->Get("check_source") << "\n"
	   << "\t" << "last_check=" << static_cast<long>(attrs->Get("last_check")) << "\n"
	   << "\t" << "next_check=" << static_cast<long>(attrs->Get("next_check")) << "\n"
	   << "\t" << "current_attempt=" << attrs->Get("current_attempt") << "\n"
	   << "\t" << "max_attempts=" << attrs->Get("max_attempts") << "\n"
	   << "\t" << "last_state_change=" << static_cast<long>(attrs->Get("last_state_change")) << "\n"
	   << "\t" << "last_hard_state_change=" << static_cast<long>(attrs->Get("last_hard_state_change")) << "\n"
	   << "\t" << "last_time_ok=" << static_cast<long>(attrs->Get("last_time_ok")) << "\n"
	   << "\t" << "last_time_warn=" << static_cast<long>(attrs->Get("last_time_warn")) << "\n"
	   << "\t" << "last_time_critical=" << static_cast<long>(attrs->Get("last_time_critical")) << "\n"
	   << "\t" << "last_time_unknown=" << static_cast<long>(attrs->Get("last_time_unknown")) << "\n"
	   << "\t" << "last_update=" << static_cast<long>(attrs->Get("last_update")) << "\n"
	   << "\t" << "notifications_enabled=" << attrs->Get("notifications_enabled") << "\n"
	   << "\t" << "active_checks_enabled=" << attrs->Get("active_checks_enabled") << "\n"
	   << "\t" << "passive_checks_enabled=" << attrs->Get("passive_checks_enabled") << "\n"
	   << "\t" << "flap_detection_enabled=" << attrs->Get("flap_detection_enabled") << "\n"
	   << "\t" << "is_flapping=" << attrs->Get("is_flapping") << "\n"
	   << "\t" << "percent_state_change=" << attrs->Get("percent_state_change") << "\n"
	   << "\t" << "problem_has_been_acknowledged=" << attrs->Get("problem_has_been_acknowledged") << "\n"
	   << "\t" << "acknowledgement_type=" << attrs->Get("acknowledgement_type") << "\n"
	   << "\t" << "acknowledgement_end_time=" << attrs->Get("acknowledgement_end_time") << "\n"
	   << "\t" << "scheduled_downtime_depth=" << attrs->Get("scheduled_downtime_depth") << "\n"
	   << "\t" << "last_notification=" << static_cast<long>(attrs->Get("last_notification")) << "\n"
	   << "\t" << "next_notification=" << static_cast<long>(attrs->Get("next_notification")) << "\n"
	   << "\t" << "current_notification_number=" << attrs->Get("current_notification_number") << "\n"
	   << "\t" << "modified_attributes=" << attrs->Get("modified_attributes") << "\n";
}

void StatusDataWriter::DumpServiceStatus(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	fp << "servicestatus {" << "\n"
	   << "\t" << "host_name=" << host->GetName() << "\n"
	   << "\t" << "service_description=" << service->GetShortName() << "\n";

	{
		ObjectLock olock(service);
		DumpServiceStatusAttrs(fp, service, CompatTypeService);
	}

	fp << "\t" << "}" << "\n"
	   << "\n";

	DumpDowntimes(fp, service, CompatTypeService);
	DumpComments(fp, service, CompatTypeService);
}

void StatusDataWriter::DumpServiceObject(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

        String check_period_str;
        TimePeriod::Ptr check_period = service->GetCheckPeriod();
        if (check_period)
                check_period_str = check_period->GetName();
        else
                check_period_str = "24x7";

	double notification_interval = -1;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
			notification_interval = notification->GetNotificationInterval();
	}

	if (notification_interval == -1)
		notification_interval = 60;

	{
		ObjectLock olock(service);

		fp << "define service {" << "\n"
		   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
		   << "\t" << "service_description" << "\t" << service->GetShortName() << "\n"
		   << "\t" << "display_name" << "\t" << service->GetDisplayName() << "\n"
		   << "\t" << "check_period" << "\t" << check_period_str << "\n"
		   << "\t" << "check_interval" << "\t" << service->GetCheckInterval() / 60.0 << "\n"
		   << "\t" << "retry_interval" << "\t" << service->GetRetryInterval() / 60.0 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << service->GetMaxCheckAttempts() << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << (service->GetEnableActiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << (service->GetEnablePassiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "flap_detection_enabled" << "\t" << (service->GetEnableFlapping() ? 1 : 0) << "\n"
		   << "\t" << "is_volatile" << "\t" << (service->IsVolatile() ? 1 : 0) << "\n"
		   << "\t" << "notifications_enabled" << "\t" << (service->GetEnableNotifications() ? 1 : 0) << "\n"
		   << "\t" << "notification_options" << "\t" << "u,w,c,r" << "\n"
   		   << "\t" << "notification_interval" << "\t" << notification_interval / 60.0 << "\n"
		   << "\t" << "event_handler_enabled" << "\t" << (service->GetEnableEventHandler() ? 1 : 0) << "\n";

		CheckCommand::Ptr checkcommand = service->GetCheckCommand();
		if (checkcommand)
			fp << "\t" << "check_command" << "\t" << "check_" << checkcommand->GetName() << "\n";

		EventCommand::Ptr eventcommand = service->GetEventCommand();
		if (eventcommand)
			fp << "\t" << "event_handler" << "\t" << "event_" << eventcommand->GetName() << "\n";

                TimePeriod::Ptr check_period = service->GetCheckPeriod();
                if (check_period)
                        fp << "\t" << "check_period" << "\t" << check_period->GetName() << "\n";

                fp << "\t" << "contacts" << "\t";
                DumpNameList(fp, CompatUtility::GetServiceNotificationUsers(service));
                fp << "\n";

                fp << "\t" << "contact_groups" << "\t";
                DumpNameList(fp, CompatUtility::GetServiceNotificationUserGroups(service));
                fp << "\n";

                fp << "\t" << "initial_state" << "\t" << "o" << "\n"
                   << "\t" << "low_flap_threshold" << "\t" << service->GetFlappingThreshold() << "\n"
                   << "\t" << "high_flap_threshold" << "\t" << service->GetFlappingThreshold() << "\n"
                   << "\t" << "process_perf_data" << "\t" << 1 << "\n"
                   << "\t" << "check_freshness" << "\t" << 1 << "\n";
	}

	fp << "\t" << "service_groups" << "\t";
	bool first = true;

        Array::Ptr groups = service->GetGroups();

        if (groups) {
                ObjectLock olock(groups);

                BOOST_FOREACH(const String& name, groups) {
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

	fp << "\t" << "}" << "\n"
	   << "\n";

	BOOST_FOREACH(const Service::Ptr& parent, service->GetParentServices()) {
		Host::Ptr host = service->GetHost();

		if (!host)
			continue;

		Host::Ptr parent_host = parent->GetHost();

		if (!parent_host)
			continue;

		fp << "define servicedependency {" << "\n"
		   << "\t" << "dependent_host_name" << "\t" << host->GetName() << "\n"
		   << "\t" << "dependent_service_description" << "\t" << service->GetShortName() << "\n"
		   << "\t" << "host_name" << "\t" << parent_host->GetName() << "\n"
		   << "\t" << "service_description" << "\t" << parent->GetShortName() << "\n"
		   << "\t" << "execution_failure_criteria" << "\t" << "n" << "\n"
		   << "\t" << "notification_failure_criteria" << "\t" << "w,u,c" << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void StatusDataWriter::DumpCustomAttributes(std::ostream& fp, const DynamicObject::Ptr& object)
{
	Dictionary::Ptr custom = object->GetCustom();

	if (!custom)
		return;

	ObjectLock olock(custom);
	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), custom) {
		fp << "\t";

		if (key != "notes" && key != "action_url" && key != "notes_url" &&
		    key != "icon_image" && key != "icon_image_alt" && key != "statusmap_image" && "2d_coords")
			fp << "_";

		fp << key << "\t" << Convert::ToString(value) << "\n";
	}
}

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void StatusDataWriter::StatusTimerHandler(void)
{
	Log(LogInformation, "compat", "Writing compat status information");

	String statuspath = GetStatusPath();
	String objectspath = GetObjectsPath();
	String statuspathtmp = statuspath + ".tmp"; /* XXX make this a global definition */
	String objectspathtmp = objectspath + ".tmp";

	std::ofstream statusfp;
	statusfp.open(statuspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	statusfp << std::fixed;

	statusfp << "# Icinga status file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	statusfp << "info {" << "\n"
		 << "\t" << "created=" << Utility::GetTime() << "\n"
		 << "\t" << "version=" << Application::GetVersion() << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	statusfp << "programstatus {" << "\n"
		 << "\t" << "icinga_pid=" << Utility::GetPid() << "\n"
		 << "\t" << "daemon_mode=1" << "\n"
		 << "\t" << "program_start=" << static_cast<long>(IcingaApplication::GetInstance()->GetStartTime()) << "\n"
		 << "\t" << "active_service_checks_enabled=" << (IcingaApplication::GetInstance()->GetEnableChecks() ? 1 : 0) << "\n"
		 << "\t" << "passive_service_checks_enabled=1" << "\n"
		 << "\t" << "active_host_checks_enabled=1" << "\n"
		 << "\t" << "passive_host_checks_enabled=1" << "\n"
		 << "\t" << "check_service_freshness=1" << "\n"
		 << "\t" << "check_host_freshness=1" << "\n"
		 << "\t" << "enable_notifications=" << (IcingaApplication::GetInstance()->GetEnableNotifications() ? 1 : 0) << "\n"
		 << "\t" << "enable_flap_detection=" << (IcingaApplication::GetInstance()->GetEnableFlapping() ? 1 : 0) << "\n"
		 << "\t" << "enable_failure_prediction=0" << "\n"
		 << "\t" << "process_performance_data=" << (IcingaApplication::GetInstance()->GetEnablePerfdata() ? 1 : 0) << "\n"
		 << "\t" << "active_scheduled_service_check_stats=" << CIB::GetActiveChecksStatistics(60) << "," << CIB::GetActiveChecksStatistics(5 * 60) << "," << CIB::GetActiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "passive_service_check_stats=" << CIB::GetPassiveChecksStatistics(60) << "," << CIB::GetPassiveChecksStatistics(5 * 60) << "," << CIB::GetPassiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "next_downtime_id=" << Service::GetNextDowntimeID() << "\n"
		 << "\t" << "next_comment_id=" << Service::GetNextCommentID() << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	std::ofstream objectfp;
	objectfp.open(objectspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	objectfp << std::fixed;

	objectfp << "# Icinga objects cache file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpHostStatus(tempstatusfp, host);
		statusfp << tempstatusfp.str();

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpHostObject(tempobjectfp, host);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const HostGroup::Ptr& hg, DynamicType::GetObjects<HostGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define hostgroup {" << "\n"
			 << "\t" << "hostgroup_name" << "\t" << hg->GetName() << "\n";

		DumpCustomAttributes(tempobjectfp, hg);

		tempobjectfp << "\t" << "members" << "\t";
		DumpNameList(tempobjectfp, hg->GetMembers());
		tempobjectfp << "\n"
			     << "\t" << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpServiceStatus(tempstatusfp, service);
		statusfp << tempstatusfp.str();

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpServiceObject(tempobjectfp, service);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const ServiceGroup::Ptr& sg, DynamicType::GetObjects<ServiceGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define servicegroup {" << "\n"
			 << "\t" << "servicegroup_name" << "\t" << sg->GetName() << "\n";

		DumpCustomAttributes(tempobjectfp, sg);

		tempobjectfp << "\t" << "members" << "\t";

		std::vector<String> sglist;
		BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
			Host::Ptr host = service->GetHost();

			if (!host)
				continue;

			sglist.push_back(host->GetName());
			sglist.push_back(service->GetShortName());
		}

		DumpStringList(tempobjectfp, sglist);

		tempobjectfp << "\n"
			 << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const User::Ptr& user, DynamicType::GetObjects<User>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define contact {" << "\n"
			 << "\t" << "contact_name" << "\t" << user->GetName() << "\n"
			 << "\t" << "alias" << "\t" << user->GetDisplayName() << "\n"
			 << "\t" << "service_notification_options" << "\t" << "w,u,c,r,f,s" << "\n"
			 << "\t" << "host_notification_options" << "\t" << "d,u,r,f,s" << "\n"
			 << "\t" << "host_notifications_enabled" << "\t" << 1 << "\n"
			 << "\t" << "service_notifications_enabled" << "\t" << 1 << "\n"
			 << "\t" << "}" << "\n"
			 << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const UserGroup::Ptr& ug, DynamicType::GetObjects<UserGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define contactgroup {" << "\n"
			 << "\t" << "contactgroup_name" << "\t" << ug->GetName() << "\n"
			 << "\t" << "alias" << "\t" << ug->GetDisplayName() << "\n";

		tempobjectfp << "\t" << "members" << "\t";
		DumpNameList(tempobjectfp, ug->GetMembers());
		tempobjectfp << "\n"
			     << "\t" << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const Command::Ptr& command, DynamicType::GetObjects<CheckCommand>()) {
		DumpCommand(objectfp, command);
	}

	BOOST_FOREACH(const Command::Ptr& command, DynamicType::GetObjects<NotificationCommand>()) {
		DumpCommand(objectfp, command);
	}

	BOOST_FOREACH(const Command::Ptr& command, DynamicType::GetObjects<EventCommand>()) {
		DumpCommand(objectfp, command);
	}

	BOOST_FOREACH(const TimePeriod::Ptr& tp, DynamicType::GetObjects<TimePeriod>()) {
		DumpTimePeriod(objectfp, tp);
	}

	statusfp.close();
	objectfp.close();

#ifdef _WIN32
	_unlink(statuspath.CStr());
	_unlink(objectspath.CStr());
#endif /* _WIN32 */

	statusfp.close();
	if (rename(statuspathtmp.CStr(), statuspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(statuspathtmp));
	}

	objectfp.close();
	if (rename(objectspathtmp.CStr(), objectspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(objectspathtmp));
	}
}

void StatusDataWriter::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("status_path", m_StatusPath);
		bag->Set("objects_path", m_ObjectsPath);
	}
}

void StatusDataWriter::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_StatusPath = bag->Get("status_path");
		m_ObjectsPath = bag->Get("objects_path");
	}
}
