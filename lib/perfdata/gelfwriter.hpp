/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef GELFWRITER_H
#define GELFWRITER_H

#include "perfdata/gelfwriter-ti.hpp"
#include "perfdata/perfdatawriterconnection.hpp"
#include "icinga/checkable.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"

namespace icinga
{

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

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	PerfdataWriterConnection::Ptr m_Connection;
	WorkQueue m_WorkQueue{10000000, 1};

	boost::signals2::connection m_HandleCheckResults, m_HandleNotifications, m_HandleStateChanges;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void NotificationToUserHandler(const Checkable::Ptr& checkable, NotificationType notificationType, const CheckResult::Ptr& cr,
		const String& author, const String& commentText, const String& commandName);
	void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);

	String ComposeGelfMessage(const Dictionary::Ptr& fields, const String& source, double ts);
	void SendLogMessage(const Checkable::Ptr& checkable, const String& gelfMessage);

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* GELFWRITER_H */
