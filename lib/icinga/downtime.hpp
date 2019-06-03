/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DOWNTIME_H
#define DOWNTIME_H

#include "icinga/i2-icinga.hpp"
#include "icinga/downtime-ti.hpp"
#include "icinga/checkable-ti.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

enum DowntimeChildOptions
{
	DowntimeNoChildren,
	DowntimeTriggeredChildren,
	DowntimeNonTriggeredChildren
};

/**
 * A downtime.
 *
 * @ingroup icinga
 */
class Downtime final : public ObjectImpl<Downtime>
{
public:
	DECLARE_OBJECT(Downtime);
	DECLARE_OBJECTNAME(Downtime);

	static boost::signals2::signal<void (const Downtime::Ptr&)> OnDowntimeAdded;
	static boost::signals2::signal<void (const Downtime::Ptr&)> OnDowntimeRemoved;
	static boost::signals2::signal<void (const Downtime::Ptr&)> OnDowntimeStarted;
	static boost::signals2::signal<void (const Downtime::Ptr&)> OnDowntimeTriggered;

	intrusive_ptr<Checkable> GetCheckable() const;

	bool IsInEffect() const;
	bool IsTriggered() const;
	bool IsExpired() const;
	bool HasValidConfigOwner() const;

	static void StaticInitialize();

	static int GetNextDowntimeID();

	static String AddDowntime(const intrusive_ptr<Checkable>& checkable, const String& author,
		const String& comment, double startTime, double endTime, bool fixed,
		const String& triggeredBy, double duration, const String& scheduledDowntime = String(),
		const String& scheduledBy = String(), const String& id = String(),
		const MessageOrigin::Ptr& origin = nullptr);

	static void RemoveDowntime(const String& id, bool cancelled, bool expired = false, const MessageOrigin::Ptr& origin = nullptr);

	void TriggerDowntime();

	static String GetDowntimeIDFromLegacyID(int id);

	static DowntimeChildOptions ChildOptionsFromValue(const Value& options);

protected:
	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

	void ValidateStartTime(const Lazy<Timestamp>& lvalue, const ValidationUtils& utils) override;
	void ValidateEndTime(const Lazy<Timestamp>& lvalue, const ValidationUtils& utils) override;

private:
	ObjectImpl<Checkable>::Ptr m_Checkable;

	bool CanBeTriggered();

	static void DowntimesStartTimerHandler();
	static void DowntimesExpireTimerHandler();
};

}

#endif /* DOWNTIME_H */
