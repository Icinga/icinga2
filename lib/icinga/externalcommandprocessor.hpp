/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXTERNALCOMMANDPROCESSOR_H
#define EXTERNALCOMMANDPROCESSOR_H

#include "icinga/i2-icinga.hpp"
#include "icinga/command.hpp"
#include "base/string.hpp"
#include <boost/signals2.hpp>
#include <vector>

namespace icinga
{

typedef std::function<void (double, const std::vector<String>& arguments)> ExternalCommandCallback;

struct ExternalCommandInfo
{
	ExternalCommandCallback Callback;
	size_t MinArgs;
	size_t MaxArgs;
};

class ExternalCommandProcessor {
public:
	static void Execute(const String& line);
	static void Execute(double time, const String& command, const std::vector<String>& arguments);

	static boost::signals2::signal<void(double, const String&, const std::vector<String>&)> OnNewExternalCommand;

private:
	ExternalCommandProcessor();

	static void ExecuteFromFile(const String& line, std::deque< std::vector<String> >& file_queue);

	static void ProcessHostCheckResult(double time, const std::vector<String>& arguments);
	static void ProcessServiceCheckResult(double time, const std::vector<String>& arguments);
	static void ScheduleHostCheck(double time, const std::vector<String>& arguments);
	static void ScheduleForcedHostCheck(double time, const std::vector<String>& arguments);
	static void ScheduleSvcCheck(double time, const std::vector<String>& arguments);
	static void ScheduleForcedSvcCheck(double time, const std::vector<String>& arguments);
	static void EnableHostCheck(double time, const std::vector<String>& arguments);
	static void DisableHostCheck(double time, const std::vector<String>& arguments);
	static void EnableSvcCheck(double time, const std::vector<String>& arguments);
	static void DisableSvcCheck(double time, const std::vector<String>& arguments);
	static void ShutdownProcess(double time, const std::vector<String>& arguments);
	static void RestartProcess(double time, const std::vector<String>& arguments);
	static void ScheduleForcedHostSvcChecks(double time, const std::vector<String>& arguments);
	static void ScheduleHostSvcChecks(double time, const std::vector<String>& arguments);
	static void EnableHostSvcChecks(double time, const std::vector<String>& arguments);
	static void DisableHostSvcChecks(double time, const std::vector<String>& arguments);
	static void AcknowledgeSvcProblem(double time, const std::vector<String>& arguments);
	static void AcknowledgeSvcProblemExpire(double time, const std::vector<String>& arguments);
	static void RemoveSvcAcknowledgement(double time, const std::vector<String>& arguments);
	static void AcknowledgeHostProblem(double time, const std::vector<String>& arguments);
	static void AcknowledgeHostProblemExpire(double time, const std::vector<String>& arguments);
	static void RemoveHostAcknowledgement(double time, const std::vector<String>& arguments);
	static void EnableHostgroupSvcChecks(double time, const std::vector<String>& arguments);
	static void DisableHostgroupSvcChecks(double time, const std::vector<String>& arguments);
	static void EnableServicegroupSvcChecks(double time, const std::vector<String>& arguments);
	static void DisableServicegroupSvcChecks(double time, const std::vector<String>& arguments);
	static void EnablePassiveHostChecks(double time, const std::vector<String>& arguments);
	static void DisablePassiveHostChecks(double time, const std::vector<String>& arguments);
	static void EnablePassiveSvcChecks(double time, const std::vector<String>& arguments);
	static void DisablePassiveSvcChecks(double time, const std::vector<String>& arguments);
	static void EnableServicegroupPassiveSvcChecks(double time, const std::vector<String>& arguments);
	static void DisableServicegroupPassiveSvcChecks(double time, const std::vector<String>& arguments);
	static void EnableHostgroupPassiveSvcChecks(double time, const std::vector<String>& arguments);
	static void DisableHostgroupPassiveSvcChecks(double time, const std::vector<String>& arguments);
	static void ProcessFile(double time, const std::vector<String>& arguments);
	static void ScheduleSvcDowntime(double time, const std::vector<String>& arguments);
	static void DelSvcDowntime(double time, const std::vector<String>& arguments);
	static void ScheduleHostDowntime(double time, const std::vector<String>& arguments);
	static void ScheduleAndPropagateHostDowntime(double, const std::vector<String>& arguments);
	static void ScheduleAndPropagateTriggeredHostDowntime(double, const std::vector<String>& arguments);
	static void DelHostDowntime(double time, const std::vector<String>& arguments);
	static void DelDowntimeByHostName(double, const std::vector<String>& arguments);
	static void ScheduleHostSvcDowntime(double time, const std::vector<String>& arguments);
	static void ScheduleHostgroupHostDowntime(double time, const std::vector<String>& arguments);
	static void ScheduleHostgroupSvcDowntime(double time, const std::vector<String>& arguments);
	static void ScheduleServicegroupHostDowntime(double time, const std::vector<String>& arguments);
	static void ScheduleServicegroupSvcDowntime(double time, const std::vector<String>& arguments);
	static void AddHostComment(double time, const std::vector<String>& arguments);
	static void DelHostComment(double time, const std::vector<String>& arguments);
	static void AddSvcComment(double time, const std::vector<String>& arguments);
	static void DelSvcComment(double time, const std::vector<String>& arguments);
	static void DelAllHostComments(double time, const std::vector<String>& arguments);
	static void DelAllSvcComments(double time, const std::vector<String>& arguments);
	static void SendCustomHostNotification(double time, const std::vector<String>& arguments);
	static void SendCustomSvcNotification(double time, const std::vector<String>& arguments);
	static void DelayHostNotification(double time, const std::vector<String>& arguments);
	static void DelaySvcNotification(double time, const std::vector<String>& arguments);
	static void EnableHostNotifications(double time, const std::vector<String>& arguments);
	static void DisableHostNotifications(double time, const std::vector<String>& arguments);
	static void EnableSvcNotifications(double time, const std::vector<String>& arguments);
	static void DisableSvcNotifications(double time, const std::vector<String>& arguments);
	static void EnableHostSvcNotifications(double, const std::vector<String>& arguments);
	static void DisableHostSvcNotifications(double, const std::vector<String>& arguments);
	static void DisableHostgroupHostChecks(double, const std::vector<String>& arguments);
	static void DisableHostgroupPassiveHostChecks(double, const std::vector<String>& arguments);
	static void DisableServicegroupHostChecks(double, const std::vector<String>& arguments);
	static void DisableServicegroupPassiveHostChecks(double, const std::vector<String>& arguments);
	static void EnableHostgroupHostChecks(double, const std::vector<String>& arguments);
	static void EnableHostgroupPassiveHostChecks(double, const std::vector<String>& arguments);
	static void EnableServicegroupHostChecks(double, const std::vector<String>& arguments);
	static void EnableServicegroupPassiveHostChecks(double, const std::vector<String>& arguments);
	static void EnableSvcFlapping(double time, const std::vector<String>& arguments);
	static void DisableSvcFlapping(double time, const std::vector<String>& arguments);
	static void EnableHostFlapping(double time, const std::vector<String>& arguments);
	static void DisableHostFlapping(double time, const std::vector<String>& arguments);
	static void EnableNotifications(double time, const std::vector<String>& arguments);
	static void DisableNotifications(double time, const std::vector<String>& arguments);
	static void EnableFlapDetection(double time, const std::vector<String>& arguments);
	static void DisableFlapDetection(double time, const std::vector<String>& arguments);
	static void EnableEventHandlers(double time, const std::vector<String>& arguments);
	static void DisableEventHandlers(double time, const std::vector<String>& arguments);
	static void EnablePerformanceData(double time, const std::vector<String>& arguments);
	static void DisablePerformanceData(double time, const std::vector<String>& arguments);
	static void StartExecutingSvcChecks(double time, const std::vector<String>& arguments);
	static void StopExecutingSvcChecks(double time, const std::vector<String>& arguments);
	static void StartExecutingHostChecks(double time, const std::vector<String>& arguments);
	static void StopExecutingHostChecks(double time, const std::vector<String>& arguments);

	static void ChangeNormalSvcCheckInterval(double time, const std::vector<String>& arguments);
	static void ChangeNormalHostCheckInterval(double time, const std::vector<String>& arguments);
	static void ChangeRetrySvcCheckInterval(double time, const std::vector<String>& arguments);
	static void ChangeRetryHostCheckInterval(double time, const std::vector<String>& arguments);
	static void EnableHostEventHandler(double time, const std::vector<String>& arguments);
	static void DisableHostEventHandler(double time, const std::vector<String>& arguments);
	static void EnableSvcEventHandler(double time, const std::vector<String>& arguments);
	static void DisableSvcEventHandler(double time, const std::vector<String>& arguments);
	static void ChangeHostEventHandler(double time, const std::vector<String>& arguments);
	static void ChangeSvcEventHandler(double time, const std::vector<String>& arguments);
	static void ChangeHostCheckCommand(double time, const std::vector<String>& arguments);
	static void ChangeSvcCheckCommand(double time, const std::vector<String>& arguments);
	static void ChangeMaxHostCheckAttempts(double time, const std::vector<String>& arguments);
	static void ChangeMaxSvcCheckAttempts(double time, const std::vector<String>& arguments);
	static void ChangeHostCheckTimeperiod(double time, const std::vector<String>& arguments);
	static void ChangeSvcCheckTimeperiod(double time, const std::vector<String>& arguments);
	static void ChangeCustomHostVar(double time, const std::vector<String>& arguments);
	static void ChangeCustomSvcVar(double time, const std::vector<String>& arguments);
	static void ChangeCustomUserVar(double time, const std::vector<String>& arguments);
	static void ChangeCustomCheckcommandVar(double time, const std::vector<String>& arguments);
	static void ChangeCustomEventcommandVar(double time, const std::vector<String>& arguments);
	static void ChangeCustomNotificationcommandVar(double time, const std::vector<String>& arguments);

	static void EnableHostgroupHostNotifications(double time, const std::vector<String>& arguments);
	static void EnableHostgroupSvcNotifications(double time, const std::vector<String>& arguments);
	static void DisableHostgroupHostNotifications(double time, const std::vector<String>& arguments);
	static void DisableHostgroupSvcNotifications(double time, const std::vector<String>& arguments);
	static void EnableServicegroupHostNotifications(double time, const std::vector<String>& arguments);
	static void EnableServicegroupSvcNotifications(double time, const std::vector<String>& arguments);
	static void DisableServicegroupHostNotifications(double time, const std::vector<String>& arguments);
	static void DisableServicegroupSvcNotifications(double time, const std::vector<String>& arguments);

private:
	static void ChangeCustomCommandVarInternal(const Command::Ptr& command, const String& name, const Value& value);

	static void RegisterCommand(const String& command, const ExternalCommandCallback& callback, size_t minArgs = 0, size_t maxArgs = UINT_MAX);
	static void RegisterCommands();

	static boost::mutex& GetMutex();
	static std::map<String, ExternalCommandInfo>& GetCommands();

};

}

#endif /* EXTERNALCOMMANDPROCESSOR_H */
