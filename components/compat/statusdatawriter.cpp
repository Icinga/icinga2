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
#include "base/context.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
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

	m_StatusTimer = make_shared<Timer>();
	m_StatusTimer->SetInterval(GetUpdateInterval());
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&StatusDataWriter::StatusTimerHandler, this));
	m_StatusTimer->Start();
	m_StatusTimer->Reschedule(0);

	Utility::QueueAsyncCallback(boost::bind(&StatusDataWriter::UpdateObjectsCache, this));
}

void StatusDataWriter::DumpComments(std::ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Service::Ptr service;
	Dictionary::Ptr comments = owner->GetComments();

	Host::Ptr host = owner->GetHost();

	ObjectLock olock(comments);

	BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
		Comment::Ptr comment = kv.second;

		if (comment->IsExpired())
			continue;

		if (type == CompatTypeHost)
			fp << "hostcomment {" << "\n";
		else
			fp << "servicecomment {" << "\n"
			   << "\t" << "service_description=" << owner->GetShortName() << "\n";

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
		BOOST_FOREACH(const Dictionary::Pair& kv, ranges) {
			fp << "\t" << kv.first << "\t" << kv.second << "\n";
		}
	}

	fp << "\t" "}" "\n"
	      "\n";
}

void StatusDataWriter::DumpCommand(std::ostream& fp, const Command::Ptr& command)
{
	fp << "define command {" "\n"
	      "\t" "command_name\t";


	if (command->GetType() == DynamicType::GetByName("CheckCommand"))
		fp << "check_";
	else if (command->GetType() == DynamicType::GetByName("NotificationCommand"))
		fp << "notification_";
	else if (command->GetType() == DynamicType::GetByName("EventCommand"))
		fp << "event_";

	fp << command->GetName() << "\n";

	fp << "\t" "command_line" "\t" << CompatUtility::GetCommandLine(command);

	fp << "\n" "\t" "}" "\n"
	      "\n";
}

void StatusDataWriter::DumpDowntimes(std::ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Host::Ptr host = owner->GetHost();

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	ObjectLock olock(downtimes);

	BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
		Downtime::Ptr downtime = kv.second;

		if (downtime->IsExpired())
			continue;

		if (type == CompatTypeHost)
			fp << "hostdowntime {" "\n";
		else
			fp << "servicedowntime {" << "\n"
			      "\t" "service_description=" << owner->GetShortName() << "\n";

		Downtime::Ptr triggeredByObj = Service::GetDowntimeByID(downtime->GetTriggeredBy());
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
		      "\t" "is_in_effect=" << (downtime->IsActive() ? 1 : 0) << "\n"
		      "\t" "author=" << downtime->GetAuthor() << "\n"
		      "\t" "comment=" << downtime->GetComment() << "\n"
		      "\t" "trigger_time=" << downtime->GetTriggerTime() << "\n"
		      "\t" "}" "\n"
		      "\n";
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
	fp << "\t" "last_time_up=" << host->GetLastStateUp() << "\n"
	      "\t" "last_time_down=" << host->GetLastStateDown() << "\n"
	      "\t" "last_time_unreachable=" << host->GetLastStateUnreachable() << "\n";

	fp << "\t" "}" "\n"
	      "\n";

	if (hc) {
		DumpDowntimes(fp, hc, CompatTypeHost);
		DumpComments(fp, hc, CompatTypeHost);
	}
}

void StatusDataWriter::DumpHostObject(std::ostream& fp, const Host::Ptr& host)
{
	fp << "define host {" "\n"
	      "\t" "host_name" "\t" << host->GetName() << "\n"
	      "\t" "display_name" "\t" << host->GetDisplayName() << "\n"
	      "\t" "alias" "\t" << host->GetDisplayName() << "\n"
	      "\t" "address" "\t" << CompatUtility::GetHostAddress(host) << "\n"
	      "\t" "address6" "\t" << CompatUtility::GetHostAddress6(host) << "\n"
	      "\t" "notes" "\t" << CompatUtility::GetCustomAttributeConfig(host, "notes") << "\n"
	      "\t" "notes_url" "\t" << CompatUtility::GetCustomAttributeConfig(host, "notes_url") << "\n"
	      "\t" "action_url" "\t" << CompatUtility::GetCustomAttributeConfig(host, "action_url") << "\n"
	      "\t" "icon_image" "\t" << CompatUtility::GetCustomAttributeConfig(host, "icon_image") << "\n"
	      "\t" "icon_image_alt" "\t" << CompatUtility::GetCustomAttributeConfig(host, "icon_image_alt") << "\n"
	      "\t" "statusmap_image" "\t" << CompatUtility::GetCustomAttributeConfig(host, "statusmap_image") << "\n";

	std::set<Host::Ptr> parents = host->GetParentHosts();

	if (!parents.empty()) {
		fp << "\t" "parents" "\t";
		DumpNameList(fp, parents);
		fp << "\n";
	}

	Service::Ptr hc = host->GetCheckService();
	if (hc) {
		ObjectLock olock(hc);

		fp << "\t" "check_interval" "\t" << CompatUtility::GetServiceCheckInterval(hc) << "\n"
		      "\t" "retry_interval" "\t" << CompatUtility::GetServiceRetryInterval(hc) << "\n"
		      "\t" "max_check_attempts" "\t" << hc->GetMaxCheckAttempts() << "\n"
		      "\t" "active_checks_enabled" "\t" << CompatUtility::GetServiceActiveChecksEnabled(hc) << "\n"
		      "\t" "passive_checks_enabled" "\t" << CompatUtility::GetServicePassiveChecksEnabled(hc) << "\n"
		      "\t" "notifications_enabled" "\t" << CompatUtility::GetServiceNotificationsEnabled(hc) << "\n"
		      "\t" "notification_options" "\t" << "d,u,r" << "\n"
		      "\t" "notification_interval" "\t" << CompatUtility::GetServiceNotificationNotificationInterval(hc) << "\n"
		      "\t" "event_handler_enabled" "\t" << CompatUtility::GetServiceEventHandlerEnabled(hc) << "\n";

		CheckCommand::Ptr checkcommand = hc->GetCheckCommand();
		if (checkcommand)
			fp << "\t" "check_command" "\t" "check_" << checkcommand->GetName() << "\n";

		EventCommand::Ptr eventcommand = hc->GetEventCommand();
		if (eventcommand)
			fp << "\t" "event_handler" "\t" "event_" << eventcommand->GetName() << "\n";

		fp << "\t" "check_period" "\t" << CompatUtility::GetServiceCheckPeriod(hc) << "\n";

		fp << "\t" "contacts" "\t";
		DumpNameList(fp, CompatUtility::GetServiceNotificationUsers(hc));
		fp << "\n";

		fp << "\t" "contact_groups" "\t";
		DumpNameList(fp, CompatUtility::GetServiceNotificationUserGroups(hc));
		fp << "\n";

		fp << "\t" << "initial_state" "\t" "o" "\n"
		      "\t" "low_flap_threshold" "\t" << hc->GetFlappingThreshold() << "\n"
		      "\t" "high_flap_threshold" "\t" << hc->GetFlappingThreshold() << "\n"
		      "\t" "process_perf_data" "\t" "1" "\n"
		      "\t" "check_freshness" "\t" "1" "\n";

	} else {
		fp << "\t" << "check_interval" "\t" "60" "\n"
		      "\t" "retry_interval" "\t" "60" "\n"
		      "\t" "max_check_attempts" "\t" "1" "\n"
		      "\t" "active_checks_enabled" "\t" "0" << "\n"
		      "\t" "passive_checks_enabled" "\t" "0" "\n"
		      "\t" "notifications_enabled" "\t" "0" "\n";

	}

	fp << "\t" "host_groups" "\t";
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

	fp << "\t" "}" "\n"
	      "\n";
}

void StatusDataWriter::DumpServiceStatusAttrs(std::ostream& fp, const Service::Ptr& service, CompatObjectType type)
{
	CheckResult::Ptr cr = service->GetLastCheckResult();

	fp << "\t" << "check_command=check_" << CompatUtility::GetServiceCheckCommand(service) << "\n"
	      "\t" "event_handler=event_" << CompatUtility::GetServiceEventHandler(service) << "\n"
	      "\t" "check_period=" << CompatUtility::GetServiceCheckPeriod(service) << "\n"
	      "\t" "check_interval=" << CompatUtility::GetServiceCheckInterval(service) << "\n"
	      "\t" "retry_interval=" << CompatUtility::GetServiceRetryInterval(service) << "\n"
	      "\t" "has_been_checked=" << CompatUtility::GetServiceHasBeenChecked(service) << "\n"
	      "\t" "should_be_scheduled=" << CompatUtility::GetServiceShouldBeScheduled(service) << "\n";

	if (cr) {
	   fp << "\t" << "check_execution_time=" << static_cast<double>(Service::CalculateExecutionTime(cr)) << "\n"
	         "\t" "check_latency=" << static_cast<double>(Service::CalculateLatency(cr)) << "\n";
	}

	fp << "\t" << "current_state=" << CompatUtility::GetServiceCurrentState(service) << "\n"
	      "\t" "state_type=" << service->GetStateType() << "\n"
	      "\t" "plugin_output=" << CompatUtility::GetCheckResultOutput(cr) << "\n"
	      "\t" "long_plugin_output=" << CompatUtility::GetCheckResultLongOutput(cr) << "\n"
	      "\t" "performance_data=" << CompatUtility::GetCheckResultPerfdata(cr) << "\n";

	if (cr) {
	   fp << "\t" << "check_source=" << cr->GetCheckSource() << "\n"
	         "\t" "last_check=" << static_cast<long>(cr->GetScheduleEnd()) << "\n";
	}

	fp << "\t" << "next_check=" << static_cast<long>(service->GetNextCheck()) << "\n"
	      "\t" "current_attempt=" << service->GetCheckAttempt() << "\n"
	      "\t" "max_attempts=" << service->GetMaxCheckAttempts() << "\n"
	      "\t" "last_state_change=" << static_cast<long>(service->GetLastStateChange()) << "\n"
	      "\t" "last_hard_state_change=" << static_cast<long>(service->GetLastHardStateChange()) << "\n"
	      "\t" "last_time_ok=" << static_cast<int>(service->GetLastStateOK()) << "\n"
	      "\t" "last_time_warn=" << static_cast<int>(service->GetLastStateWarning()) << "\n"
	      "\t" "last_time_critical=" << static_cast<int>(service->GetLastStateCritical()) << "\n"
	      "\t" "last_time_unknown=" << static_cast<int>(service->GetLastStateUnknown()) << "\n"
	      "\t" "last_update=" << static_cast<long>(time(NULL)) << "\n"
	      "\t" "notifications_enabled=" << CompatUtility::GetServiceNotificationsEnabled(service) << "\n"
	      "\t" "active_checks_enabled=" << CompatUtility::GetServiceActiveChecksEnabled(service) << "\n"
	      "\t" "passive_checks_enabled=" << CompatUtility::GetServicePassiveChecksEnabled(service) << "\n"
	      "\t" "flap_detection_enabled=" << CompatUtility::GetServiceFlapDetectionEnabled(service) << "\n"
	      "\t" "is_flapping=" << CompatUtility::GetServiceIsFlapping(service) << "\n"
	      "\t" "percent_state_change=" << CompatUtility::GetServicePercentStateChange(service) << "\n"
	      "\t" "problem_has_been_acknowledged=" << CompatUtility::GetServiceProblemHasBeenAcknowledged(service) << "\n"
	      "\t" "acknowledgement_type=" << CompatUtility::GetServiceAcknowledgementType(service) << "\n"
	      "\t" "acknowledgement_end_time=" << service->GetAcknowledgementExpiry() << "\n"
	      "\t" "scheduled_downtime_depth=" << service->GetDowntimeDepth() << "\n"
	      "\t" "last_notification=" << CompatUtility::GetServiceNotificationLastNotification(service) << "\n"
	      "\t" "next_notification=" << CompatUtility::GetServiceNotificationNextNotification(service) << "\n"
	      "\t" "current_notification_number=" << CompatUtility::GetServiceNotificationNotificationNumber(service) << "\n"
	      "\t" "modified_attributes=" << service->GetModifiedAttributes() << "\n";
}

void StatusDataWriter::DumpServiceStatus(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	fp << "servicestatus {" "\n"
	      "\t" "host_name=" << host->GetName() << "\n"
	      "\t" "service_description=" << service->GetShortName() << "\n";

	{
		ObjectLock olock(service);
		DumpServiceStatusAttrs(fp, service, CompatTypeService);
	}

	fp << "\t" "}" "\n"
	      "\n";

	DumpDowntimes(fp, service, CompatTypeService);
	DumpComments(fp, service, CompatTypeService);
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
		      "\t" "check_period" "\t" << CompatUtility::GetServiceCheckPeriod(service) << "\n"
		      "\t" "check_interval" "\t" << CompatUtility::GetServiceCheckInterval(service) << "\n"
		      "\t" "retry_interval" "\t" << CompatUtility::GetServiceRetryInterval(service) << "\n"
		      "\t" "max_check_attempts" "\t" << service->GetMaxCheckAttempts() << "\n"
		      "\t" "active_checks_enabled" "\t" << CompatUtility::GetServiceActiveChecksEnabled(service) << "\n"
		      "\t" "passive_checks_enabled" "\t" << CompatUtility::GetServicePassiveChecksEnabled(service) << "\n"
		      "\t" "flap_detection_enabled" "\t" << CompatUtility::GetServiceFlapDetectionEnabled(service) << "\n"
		      "\t" "is_volatile" "\t" << CompatUtility::GetServiceIsVolatile(service) << "\n"
		      "\t" "notifications_enabled" "\t" << CompatUtility::GetServiceNotificationsEnabled(service) << "\n"
		      "\t" "notification_options" "\t" << CompatUtility::GetServiceNotificationNotificationOptions(service) << "\n"
		      "\t" "notification_interval" "\t" << CompatUtility::GetServiceNotificationNotificationInterval(service) << "\n"
		      "\t" "notification_period" "\t" << CompatUtility::GetServiceNotificationNotificationPeriod(service) << "\n"
		      "\t" "event_handler_enabled" "\t" << CompatUtility::GetServiceEventHandlerEnabled(service) << "\n";

		CheckCommand::Ptr checkcommand = service->GetCheckCommand();
		if (checkcommand)
			fp << "\t" "check_command" "\t" "check_" << checkcommand->GetName() << "\n";

		EventCommand::Ptr eventcommand = service->GetEventCommand();
		if (eventcommand)
			fp << "\t" "event_handler" "\t" "event_" << eventcommand->GetName() << "\n";

                fp << "\t" "contacts" "\t";
                DumpNameList(fp, CompatUtility::GetServiceNotificationUsers(service));
                fp << "\n";

                fp << "\t" "contact_groups" "\t";
                DumpNameList(fp, CompatUtility::GetServiceNotificationUserGroups(service));
                fp << "\n";

                fp << "\t" "initial_state" "\t" "o" "\n"
                      "\t" "low_flap_threshold" "\t" << service->GetFlappingThreshold() << "\n"
                      "\t" "high_flap_threshold" "\t" << service->GetFlappingThreshold() << "\n"
                      "\t" "process_perf_data" "\t" "1" "\n"
                      "\t" "check_freshness" << "\t" "1" "\n"
		      "\t" "notes" "\t" << CompatUtility::GetCustomAttributeConfig(service, "notes") << "\n"
		      "\t" "notes_url" "\t" << CompatUtility::GetCustomAttributeConfig(service, "notes_url") << "\n"
		      "\t" "action_url" "\t" << CompatUtility::GetCustomAttributeConfig(service, "action_url") << "\n"
		      "\t" "icon_image" "\t" << CompatUtility::GetCustomAttributeConfig(service, "icon_image") << "\n"
		      "\t" "icon_image_alt" "\t" << CompatUtility::GetCustomAttributeConfig(service, "icon_image_alt") << "\n";
	}

	fp << "\t" "service_groups" "\t";
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

	fp << "\t" "}" "\n"
	      "\n";

	BOOST_FOREACH(const Service::Ptr& parent, service->GetParentServices()) {
		Host::Ptr host = service->GetHost();

		Host::Ptr parent_host = parent->GetHost();

		if (!parent_host)
			continue;

		fp << "define servicedependency {" "\n"
		      "\t" "dependent_host_name" "\t" << host->GetName() << "\n"
		      "\t" "dependent_service_description" "\t" << service->GetShortName() << "\n"
		      "\t" "host_name" "\t" << parent_host->GetName() << "\n"
		      "\t" "service_description" "\t" << parent->GetShortName() << "\n"
		      "\t" "execution_failure_criteria" "\t" "n" "\n"
		      "\t" "notification_failure_criteria" "\t" "w,u,c" "\n"
		      "\t" "}" "\n"
		      "\n";
	}
}

void StatusDataWriter::DumpCustomAttributes(std::ostream& fp, const DynamicObject::Ptr& object)
{
	Dictionary::Ptr custom = object->GetCustom();

	if (!custom)
		return;

	ObjectLock olock(custom);
	BOOST_FOREACH(const Dictionary::Pair& kv, custom) {
		if (!kv.first.IsEmpty()) {
			fp << "\t";

			if (kv.first != "notes" && kv.first != "action_url" && kv.first != "notes_url" &&
			    kv.first != "icon_image" && kv.first != "icon_image_alt" && kv.first != "statusmap_image" && kv.first != "2d_coords")
				fp << "_";

			fp << kv.first << "\t" << kv.second << "\n";
		}
	}
}

void StatusDataWriter::UpdateObjectsCache(void)
{
	CONTEXT("Writing objects.cache file");

	String objectspath = GetObjectsPath();
	String objectspathtmp = objectspath + ".tmp";

	std::ofstream objectfp;
	objectfp.open(objectspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	objectfp << std::fixed;

	objectfp << "# Icinga objects cache file" "\n"
		    "# This file is auto-generated. Do not modify this file." "\n"
		    "\n";

	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpHostObject(tempobjectfp, host);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const HostGroup::Ptr& hg, DynamicType::GetObjects<HostGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define hostgroup {" "\n"
				"\t" "hostgroup_name" "\t" << hg->GetName() << "\n";

		DumpCustomAttributes(tempobjectfp, hg);

		tempobjectfp << "\t" "members" "\t";
		DumpNameList(tempobjectfp, hg->GetMembers());
		tempobjectfp << "\n"
			        "\t" "}" "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpServiceObject(tempobjectfp, service);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const ServiceGroup::Ptr& sg, DynamicType::GetObjects<ServiceGroup>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define servicegroup {" "\n"
			 	"\t" "servicegroup_name" "\t" << sg->GetName() << "\n";

		DumpCustomAttributes(tempobjectfp, sg);

		tempobjectfp << "\t" "members" "\t";

		std::vector<String> sglist;
		BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
			Host::Ptr host = service->GetHost();

			sglist.push_back(host->GetName());
			sglist.push_back(service->GetShortName());
		}

		DumpStringList(tempobjectfp, sglist);

		tempobjectfp << "\n"
			 	"}" "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const User::Ptr& user, DynamicType::GetObjects<User>()) {
		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define contact {" "\n"
				"\t" "contact_name" "\t" << user->GetName() << "\n"
				"\t" "alias" "\t" << user->GetDisplayName() << "\n"
				"\t" "service_notification_options" "\t" "w,u,c,r,f,s" "\n"
			 	"\t" "host_notification_options""\t" "d,u,r,f,s" "\n"
				"\t" "host_notifications_enabled" "\t" "1" "\n"
				"\t" "service_notifications_enabled" "\t" "1" "\n"
				"\t" "}" "\n"
				"\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const UserGroup::Ptr& ug, DynamicType::GetObjects<UserGroup>()) {
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

	objectfp.close();

#ifdef _WIN32
	_unlink(objectspath.CStr());
#endif /* _WIN32 */

	if (rename(objectspathtmp.CStr(), objectspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(objectspathtmp));
	}
}

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void StatusDataWriter::StatusTimerHandler(void)
{
	Log(LogInformation, "compat", "Writing status.dat file");

	String statuspath = GetStatusPath();
	String statuspathtmp = statuspath + ".tmp"; /* XXX make this a global definition */

	std::ofstream statusfp;
	statusfp.open(statuspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	statusfp << std::fixed;

	statusfp << "# Icinga status file" "\n"
		    "# This file is auto-generated. Do not modify this file." "\n"
		    "\n";

	statusfp << "info {" "\n"
		    "\t" "created=" << Utility::GetTime() << "\n"
		    "\t" "version=" << Application::GetVersion() << "\n"
		    "\t" "}" "\n"
		    "\n";

	statusfp << "programstatus {" "\n"
		    "\t" "icinga_pid=" << Utility::GetPid() << "\n"
		    "\t" "daemon_mode=1" "\n"
		    "\t" "program_start=" << static_cast<long>(Application::GetStartTime()) << "\n"
		    "\t" "active_service_checks_enabled=" << (IcingaApplication::GetInstance()->GetEnableChecks() ? 1 : 0) << "\n"
		    "\t" "passive_service_checks_enabled=1" "\n"
		    "\t" "active_host_checks_enabled=1" "\n"
		    "\t" "passive_host_checks_enabled=1" "\n"
		    "\t" "check_service_freshness=1" "\n"
		    "\t" "check_host_freshness=1" "\n"
		    "\t" "enable_notifications=" << (IcingaApplication::GetInstance()->GetEnableNotifications() ? 1 : 0) << "\n"
		    "\t" "enable_flap_detection=" << (IcingaApplication::GetInstance()->GetEnableFlapping() ? 1 : 0) << "\n"
		    "\t" "enable_failure_prediction=0" "\n"
		    "\t" "process_performance_data=" << (IcingaApplication::GetInstance()->GetEnablePerfdata() ? 1 : 0) << "\n"
		    "\t" "active_scheduled_service_check_stats=" << CIB::GetActiveChecksStatistics(60) << "," << CIB::GetActiveChecksStatistics(5 * 60) << "," << CIB::GetActiveChecksStatistics(15 * 60) << "\n"
		    "\t" "passive_service_check_stats=" << CIB::GetPassiveChecksStatistics(60) << "," << CIB::GetPassiveChecksStatistics(5 * 60) << "," << CIB::GetPassiveChecksStatistics(15 * 60) << "\n"
		    "\t" "next_downtime_id=" << Service::GetNextDowntimeID() << "\n"
		    "\t" "next_comment_id=" << Service::GetNextCommentID() << "\n"
		    "\t" "}" "\n"
		    "\n";

	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpHostStatus(tempstatusfp, host);
		statusfp << tempstatusfp.str();
	}

	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpServiceStatus(tempstatusfp, service);
		statusfp << tempstatusfp.str();
	}

	statusfp.close();

#ifdef _WIN32
	_unlink(statuspath.CStr());
#endif /* _WIN32 */

	if (rename(statuspathtmp.CStr(), statuspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(statuspathtmp));
	}

	Log(LogInformation, "compat", "Finished writing status.dat file");
}

