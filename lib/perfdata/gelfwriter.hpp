/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef GELFWRITER_H
#define GELFWRITER_H

#include "perfdata/gelfwriter-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include <fstream>

namespace icinga
{

enum GelfWriterEventType
{
	GelfWriterEventTypeCheckResult = 1,
	GelfWriterEventTypeStateChange = 2,
	GelfWriterEventTypeNotification = 4
};

/**
 * An Icinga Gelf writer for Graylog.
 *
 * @ingroup perfdata
 */
class GelfWriter final : public ObjectImpl<GelfWriter>
{
public:
	DECLARE_OBJECT(GelfWriter);
	DECLARE_OBJECTNAME(GelfWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	static void StaticInitialize();

	void ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	static const std::map<String, int>& GetTypeFilterMap();

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	OptionalTlsStream m_Stream;
	WorkQueue m_WorkQueue{10000000, 1};

	Timer::Ptr m_ReconnectTimer;

	static std::map<String, int> m_TypeFilterMap;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void NotificationToUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
		const User::Ptr& user, NotificationType notificationType, const CheckResult::Ptr& cr,
		const String& author, const String& commentText, const String& commandName);
	void NotificationToUserHandlerInternal(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
		const User::Ptr& user, NotificationType notification_type, const CheckResult::Ptr& cr,
		const String& author, const String& comment_text, const String& command_name);
	void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);
	void StateChangeHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);

	String ComposeGelfMessage(const Dictionary::Ptr& fields, const String& source, double ts);
	void SendLogMessage(const Checkable::Ptr& checkable, const String& gelfMessage);

	void ReconnectTimerHandler();

	void Disconnect();
	void DisconnectInternal();
	void Reconnect();
	void ReconnectInternal();

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* GELFWRITER_H */
