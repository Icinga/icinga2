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

enum DowntimeRemovalReason
{
	DowntimeExpired,
	DowntimeRemovedByUser,
	DowntimeRemovedByConfigOwner,
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
	static boost::signals2::signal<void (const Downtime::Ptr&, const String&, double, const MessageOrigin::Ptr&)> OnRemovalInfoChanged;

	intrusive_ptr<Checkable> GetCheckable() const;

	bool IsInEffect() const;
	bool IsTriggered() const;
	bool IsExpired() const;
	bool CanBeTriggered();
	bool HasValidConfigOwner() const;

	static void StaticInitialize();

	static int GetNextDowntimeID();

	static Ptr AddDowntime(const intrusive_ptr<Checkable>& checkable, const String& author,
		const String& comment, double startTime, double endTime, bool fixed,
		const Ptr& parentDowntime, double duration, const String& scheduledDowntime = String(),
		const String& scheduledBy = String(), const String& parent = String(), const String& id = String(),
		const MessageOrigin::Ptr& origin = nullptr);

	static void RemoveDowntime(const String& id, bool includeChildren, DowntimeRemovalReason removalReason,
		const String& removedBy = "", const MessageOrigin::Ptr& origin = nullptr);

	void RegisterChild(const Downtime::Ptr& downtime);
	void UnregisterChild(const Downtime::Ptr& downtime);
	std::set<Downtime::Ptr> GetChildren() const;

	void TriggerDowntime(double triggerTime);
	void SetRemovalInfo(const String& removedBy, double removeTime, const MessageOrigin::Ptr& origin = nullptr);

	void OnAllConfigLoaded() override;

	static Downtime::Ptr GetDowntimeFromLegacyID(int id);

	static DowntimeChildOptions ChildOptionsFromValue(const Value& options);

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

	void Pause() override;
	void Resume() override;

	void ValidateStartTime(const Lazy<Timestamp>& lvalue, const ValidationUtils& utils) override;
	void ValidateEndTime(const Lazy<Timestamp>& lvalue, const ValidationUtils& utils) override;

private:
	ObjectImpl<Checkable>::Ptr m_Checkable;

	std::set<Downtime::Ptr> m_Children;
	mutable std::mutex m_ChildrenMutex;

	Timer::Ptr m_CleanupTimer;

	void SetupCleanupTimer();

	static void DowntimesStartTimerHandler();
	static void DowntimesOrphanedTimerHandler();
};

}

#endif /* DOWNTIME_H */
