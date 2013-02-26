/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef EXTERNALCOMMANDPROCESSOR_H
#define EXTERNALCOMMANDPROCESSOR_H

namespace icinga
{

class I2_ICINGA_API ExternalCommandProcessor {
public:
	static void Execute(const String& line);
	static void Execute(double time, const String& command, const vector<String>& arguments);

private:
	typedef function<void (double time, const vector<String>& arguments)> Callback;

	static boost::once_flag m_InitializeOnce;
	static boost::mutex m_Mutex;
	static map<String, Callback> m_Commands;

	ExternalCommandProcessor(void);

	static void Initialize(void);

	static void RegisterCommand(const String& command, const Callback& callback);

	static void ProcessHostCheckResult(double time, const vector<String>& arguments);
	static void ProcessServiceCheckResult(double time, const vector<String>& arguments);
	static void ScheduleHostCheck(double time, const vector<String>& arguments);
	static void ScheduleForcedHostCheck(double time, const vector<String>& arguments);
	static void ScheduleSvcCheck(double time, const vector<String>& arguments);
	static void ScheduleForcedSvcCheck(double time, const vector<String>& arguments);
	static void EnableHostCheck(double time, const vector<String>& arguments);
	static void DisableHostCheck(double time, const vector<String>& arguments);
	static void EnableSvcCheck(double time, const vector<String>& arguments);
	static void DisableSvcCheck(double time, const vector<String>& arguments);
	static void ShutdownProcess(double time, const vector<String>& arguments);
	static void ScheduleForcedHostSvcChecks(double time, const vector<String>& arguments);
	static void ScheduleHostSvcChecks(double time, const vector<String>& arguments);
	static void EnableHostSvcChecks(double time, const vector<String>& arguments);
	static void DisableHostSvcChecks(double time, const vector<String>& arguments);
	static void AcknowledgeSvcProblem(double time, const vector<String>& arguments);
	static void AcknowledgeSvcProblemExpire(double time, const vector<String>& arguments);
	static void RemoveSvcAcknowledgement(double time, const vector<String>& arguments);
	static void AcknowledgeHostProblem(double time, const vector<String>& arguments);
	static void AcknowledgeHostProblemExpire(double time, const vector<String>& arguments);
	static void RemoveHostAcknowledgement(double time, const vector<String>& arguments);
	static void EnableHostgroupSvcChecks(double time, const vector<String>& arguments);
	static void DisableHostgroupSvcChecks(double time, const vector<String>& arguments);
	static void EnableServicegroupSvcChecks(double time, const vector<String>& arguments);
	static void DisableServicegroupSvcChecks(double time, const vector<String>& arguments);
	static void EnablePassiveHostChecks(double time, const vector<String>& arguments);
	static void DisablePassiveHostChecks(double time, const vector<String>& arguments);
	static void EnablePassiveSvcChecks(double time, const vector<String>& arguments);
	static void DisablePassiveSvcChecks(double time, const vector<String>& arguments);
	static void EnableServicegroupPassiveSvcChecks(double time, const vector<String>& arguments);
	static void DisableServicegroupPassiveSvcChecks(double time, const vector<String>& arguments);
	static void EnableHostgroupPassiveSvcChecks(double time, const vector<String>& arguments);
	static void DisableHostgroupPassiveSvcChecks(double time, const vector<String>& arguments);
	static void ProcessFile(double time, const vector<String>& arguments);
	static void ScheduleSvcDowntime(double time, const vector<String>& arguments);
	static void DelSvcDowntime(double time, const vector<String>& arguments);
	static void ScheduleHostDowntime(double time, const vector<String>& arguments);
	static void DelHostDowntime(double time, const vector<String>& arguments);
	static void ScheduleHostSvcDowntime(double time, const vector<String>& arguments);
	static void ScheduleHostgroupHostDowntime(double time, const vector<String>& arguments);
	static void ScheduleHostgroupSvcDowntime(double time, const vector<String>& arguments);
	static void ScheduleServicegroupHostDowntime(double time, const vector<String>& arguments);
	static void ScheduleServicegroupSvcDowntime(double time, const vector<String>& arguments);
	static void AddHostComment(double time, const vector<String>& arguments);
	static void DelHostComment(double time, const vector<String>& arguments);
	static void AddSvcComment(double time, const vector<String>& arguments);
	static void DelSvcComment(double time, const vector<String>& arguments);
	static void DelAllHostComments(double time, const vector<String>& arguments);
	static void DelAllSvcComments(double time, const vector<String>& arguments);
	static void SendCustomHostNotification(double time, const vector<String>& arguments);
	static void SendCustomSvcNotification(double time, const vector<String>& arguments);
};

}

#endif /* EXTERNALCOMMANDPROCESSOR_H */
