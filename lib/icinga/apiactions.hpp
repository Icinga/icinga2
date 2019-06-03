/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APIACTIONS_H
#define APIACTIONS_H

#include "icinga/i2-icinga.hpp"
#include "base/configobject.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class ApiActions
{
public:
	static Dictionary::Ptr ProcessCheckResult(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RescheduleCheck(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr SendCustomNotification(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DelayNotification(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr AcknowledgeProblem(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveAcknowledgement(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr AddComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ScheduleDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ShutdownProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RestartProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr GenerateTicket(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);

private:
	static Dictionary::Ptr CreateResult(int code, const String& status, const Dictionary::Ptr& additional = nullptr);
};

}

#endif /* APIACTIONS_H */
