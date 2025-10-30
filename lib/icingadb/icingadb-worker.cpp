// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "icingadb/icingadb.hpp"
#include "base/logger.hpp"
#include <vector>

using namespace icinga;

PendingQueueItem::PendingQueueItem(PendingItemKey&& id, uint32_t dirtyBits)
	: DirtyBits{dirtyBits & DirtyBitsAll}, ID{std::move(id)}, EnqueueTime{std::chrono::steady_clock::now()}
{
}

PendingConfigItem::PendingConfigItem(const ConfigObject::Ptr& obj, uint32_t bits)
	: PendingQueueItem{std::make_pair(obj, nullptr), bits}, Object{obj}
{
}
PendingDependencyGroupStateItem::PendingDependencyGroupStateItem(const DependencyGroup::Ptr& depGroup)
	: PendingQueueItem{std::make_pair(nullptr, depGroup), 0}, DepGroup{depGroup}
{
}

PendingDependencyEdgeItem::PendingDependencyEdgeItem(const DependencyGroup::Ptr& depGroup, const Checkable::Ptr& child)
	: PendingQueueItem{std::make_pair(child, depGroup), 0}, DepGroup{depGroup}, Child{child}
{
}

RelationsDeletionItem::RelationsDeletionItem(const String& id, RelationsKeyMap relations)
	: PendingQueueItem{id, 0}, Relations{std::move(relations)}
{
}

/**
 * Background worker thread procedure for processing pending items.
 *
 * This function runs in a separate thread and continuously processes pending items that have been
 * enqueued for Redis updates. It waits for new items to be added to the pending items container,
 * and processes them one at a time, ensuring that the Redis connection is active and not overloaded
 * with too many pending queries. The function also implements a delay mechanism to allow for potential
 * additional changes to be merged into the same item before processing it.
 */
void IcingaDB::PendingItemsThreadProc()
{
	using namespace std::chrono_literals;
	namespace ch = std::chrono;

	// Limits the number of pending queries the Rcon can have at any given time to reduce the memory overhead to
	// the absolute minimum necessary, since the size of the pending queue items is much smaller than the size
	// of the actual Redis queries. Thus, this will slow down the worker thread a bit from generating too many
	// Redis queries when the Redis connection is saturated.
	constexpr size_t maxPendingQueries = 128;

	// Predicate to determine whether the worker thread is allowed to process pending items.
	auto canContinue = [this] {
		if (!GetActive()) {
			return true;
		}
		return !m_PendingItems.empty() && m_RconWorker && m_RconWorker->IsConnected() && m_RconWorker->GetPendingQueryCount() < maxPendingQueries;
	};

	std::unique_lock lock(m_PendingItemsMutex);
	while (GetActive()) {
		// Even if someone notifies us, we still need to verify whether the precondition is actually fulfilled.
		// However, in case we don't receive any notification, we still want to wake up periodically on our own
		// to check whether we can proceed (e.g. the Redis connection might have become available again and there
		// was no activity on the pending items queue to trigger a notification). Thus, we use a timed wait here.
		while (!canContinue()) m_PendingItemsCV.wait_for(lock, 100ms);

		if (!GetActive()) {
			break;
		}
		if (auto retryAfter = DequeueAndProcessOne(lock); retryAfter > 0ms) {
			m_PendingItemsCV.wait_for(lock, retryAfter);
		}
	}
}

/**
 * Dequeue and process a single pending item.
 *
 * This function processes a single pending item from the pending items container. It iterates over
 * the items in insertion order and checks if the first item is old enough to be processed (at least
 * 1000ms old) unless we're being shutting down. If the item can be processed, it attempts to acquire
 * a lock on the associated config object (if applicable) and processes the item accordingly.
 *
 * If the item cannot be processed right now because it's too new, the function returns a duration
 * indicating how long to wait before retrying. Also, if no progress was made during this iteration
 * (i.e., no item was processed), it returns a short delay to avoid busy-looping.
 *
 * @param lock A unique lock on the pending items mutex (must be acquired before calling this function).
 *
 * @return A duration indicating how long to wait before retrying.
 */
std::chrono::duration<double> IcingaDB::DequeueAndProcessOne(std::unique_lock<std::mutex>& lock)
{
	using namespace std::chrono_literals;
	namespace ch = std::chrono;

	bool madeProgress = false; // Did we make any progress in this iteration?
	ch::duration<double> retryAfter{0}; // If we can't process anything right now, how long to wait before retrying?
	auto now = ch::steady_clock::now();

	auto& seqView = m_PendingItems.get<1>();
	for (auto it(seqView.begin()); it != seqView.end(); ++it) {
		if (it != seqView.begin()) {
			if (std::holds_alternative<RelationsDeletionItem>(*it)) {
				// We don't know whether the previous items are related to this deletion item or not,
				// thus we can't just process this right now when there are older items in the queue.
				// Otherwise, we might delete something that is going to be updated/created.
				break;
			}
		}

		auto age = now - std::visit([](const auto& item) { return item.EnqueueTime; }, *it);
		if (GetActive() && 1000ms > age) {
			if (it == seqView.begin()) {
				retryAfter = 1000ms - age;
			}
			break;
		}

		ConfigObject::Ptr cobj;
		if (auto* citem = std::get_if<PendingConfigItem>(&*it); citem) {
			cobj = citem->Object;
		}

		ObjectLock olock(cobj, std::defer_lock);
		if (cobj && !olock.TryLock()) {
			continue; // Can't lock the object right now, try the next one.
		}

		PendingItemVariant itemToProcess = *it;
		seqView.erase(it);
		madeProgress = true;

		lock.unlock();
		try {
			std::visit([this](const auto& item) { ProcessPendingItem(item); }, itemToProcess);
		} catch (const std::exception& ex) {
			Log(LogCritical, "IcingaDB")
				<< "Exception while processing pending item of type index '" << itemToProcess.index() << "': "
				<< DiagnosticInformation(ex, GetActive());
		}
		lock.lock();
		break;
	}

	if (!madeProgress && retryAfter == 0ms) {
		// We haven't made any progress, so give it a short delay before retrying.
		retryAfter = 10ms;
	}
	return retryAfter;
}

/**
 * Process a single pending object.
 *
 * This function processes a single pending object based on its dirty bits. It checks if the object is a
 * @c ConfigObject and performs the appropriate actions such as sending configuration updates, state updates,
 * or deletions to the Redis connection. The function handles different types of objects, including @c Checkable
 * objects, and ensures that the correct updates are sent based on the dirty bits set for the object.
 *
 * @param item The pending item containing the object and its dirty bits.
 */
void IcingaDB::ProcessPendingItem(const PendingConfigItem& item)
{
	if (item.DirtyBits & ConfigDelete) {
		String typeName = GetLowerCaseTypeNameDB(item.Object);
		m_RconWorker->FireAndForgetQueries(
			{
				{"HDEL", m_PrefixConfigObject + typeName, GetObjectIdentifier(item.Object)},
				{"HDEL", m_PrefixConfigCheckSum + typeName, GetObjectIdentifier(item.Object)},
				{
					"XADD",
					"icinga:runtime",
					"MAXLEN",
					"~",
					"1000000",
					"*",
					"redis_key",
					m_PrefixConfigObject + typeName,
					"id",
					GetObjectIdentifier(item.Object),
					"runtime_type",
					"delete"
				}
			},
			RedisConnection::QueryPriority::Config
		);
	}

	if (item.DirtyBits & ConfigUpdate) {
		std::map<String, std::vector<String>> hMSets;
		std::vector<Dictionary::Ptr> runtimeUpdates;
		CreateConfigUpdate(item.Object, GetLowerCaseTypeNameDB(item.Object), hMSets, runtimeUpdates, true);
		ExecuteRedisTransaction(m_RconWorker, hMSets, runtimeUpdates);
	}

	if (auto checkable = dynamic_pointer_cast<Checkable>(item.Object); checkable) {
		if (item.DirtyBits & FullState) {
			UpdateState(checkable, item.DirtyBits);
		}
		if (item.DirtyBits & NextUpdate) {
			SendNextUpdate(checkable);
		}
	}
}

/**
 * Process a single pending dependency group state item.
 *
 * This function processes a single pending dependency group state item by updating the dependencies
 * state for the associated dependency group. It selects any child checkable from the dependency group
 * to initiate the state update process.
 *
 * @param item The pending dependency group state item containing the dependency group.
 */
void IcingaDB::ProcessPendingItem(const PendingDependencyGroupStateItem& item) const
{
	// For dependency group state updates, we don't actually care which child triggered the update,
	// since all children share the same dependency group state. Thus, we can just pick any child to
	// start the update from.
	if (auto child = item.DepGroup->GetAnyChild(); child) {
		UpdateDependenciesState(child, item.DepGroup);
	}
}

/**
 * Process a single pending dependency edge item.
 *
 * This function fully serializes a single pending dependency edge item (child registration)
 * and sends all the resulting Redis queries in a single transaction. The dependencies (edges)
 * to serialize are determined by the dependency group and child checkable the provided item represents.
 *
 * @param item The pending dependency edge item containing the dependency group and child checkable.
 */
void IcingaDB::ProcessPendingItem(const PendingDependencyEdgeItem& item)
{
	std::vector<Dictionary::Ptr> runtimeUpdates;
	std::map<String, RedisConnection::Query> hMSets;
	InsertCheckableDependencies(item.Child, hMSets, &runtimeUpdates, item.DepGroup);
	ExecuteRedisTransaction(m_RconWorker, hMSets, runtimeUpdates);
}

/**
 * Process a single pending deletion item.
 *
 * This function processes a single pending deletion item by deleting the specified sub-keys
 * from Redis based on the provided deletion keys map. It ensures that the object's ID is
 * removed from the specified Redis keys and their corresponding checksum keys if indicated.
 *
 * @param item The pending deletion item containing the ID and deletion keys map.
 */
void IcingaDB::ProcessPendingItem(const RelationsDeletionItem& item)
{
	ASSERT(std::holds_alternative<std::string>(item.ID)); // Relation deletion items must have real IDs.

	auto id = std::get<std::string>(item.ID);
	for (auto [redisKey, hasChecksum] : item.Relations) {
		if (IsStateKey(redisKey)) {
			DeleteState(id, redisKey, hasChecksum);
		} else {
			DeleteRelationship(id, redisKey, hasChecksum);
		}
	}
}

/**
 * Enqueue a configuration object for processing in the pending objects thread.
 *
 * @param object The configuration object to be enqueued for processing.
 * @param bits The dirty bits indicating the type of changes to be processed for the object.
 */
void IcingaDB::EnqueueConfigObject(const ConfigObject::Ptr& object, uint32_t bits)
{
	if (!GetActive() || !m_RconWorker || !m_RconWorker->IsConnected()) {
		return; // No need to enqueue anything if we're not connected.
	}

	{
		std::lock_guard lock(m_PendingItemsMutex);
		if (auto [it, inserted] = m_PendingItems.insert(PendingConfigItem{object, bits}); !inserted) {
			m_PendingItems.modify(it, [bits](PendingItemVariant& itemToProcess) mutable {
				std::visit(
					[&bits](auto& item) {
						if (bits & ConfigDelete) {
							// A config delete and config update cancel each other out, and we don't need
							// to keep any state updates either, as the object is being deleted.
							item.DirtyBits &= ~(ConfigUpdate | FullState);
							bits &= ~(ConfigUpdate | FullState); // Must not add these bits either.
						} else if (bits & ConfigUpdate) {
							// A new config update cancels any pending config deletion for the same object.
							item.DirtyBits &= ~ConfigDelete;
							bits &= ~ConfigDelete;
						}
						item.DirtyBits |= bits & DirtyBitsAll;
					},
					itemToProcess
				);
			});
		}
	}
	m_PendingItemsCV.notify_one();
}

void IcingaDB::EnqueueDependencyGroupStateUpdate(const DependencyGroup::Ptr& depGroup)
{
	if (GetActive() && m_RconWorker && m_RconWorker->IsConnected()) {
		{
			std::lock_guard lock(m_PendingItemsMutex);
			m_PendingItems.insert(PendingDependencyGroupStateItem{depGroup});
		}
		m_PendingItemsCV.notify_one();
	}
}

/**
 * Enqueue the registration of a dependency child to a dependency group.
 *
 * This function adds a pending item to the queue for processing the registration of a child checkable
 * to a dependency group. If there is no active Redis connection available, this function is a no-op.
 *
 * @param depGroup The dependency group to which the child is being registered.
 * @param child The child checkable being registered to the dependency group.
 */
void IcingaDB::EnqueueDependencyChildRegistered(const DependencyGroup::Ptr& depGroup, const Checkable::Ptr& child)
{
	if (GetActive() && m_RconWorker && m_RconWorker->IsConnected()) {
		{
			std::lock_guard lock(m_PendingItemsMutex);
			m_PendingItems.insert(PendingDependencyEdgeItem{depGroup, child});
		}
		m_PendingItemsCV.notify_one();
	}
}

/**
 * Enqueue the removal of a dependency child from a dependency group.
 *
 * This function handles the removal of a child checkable from a dependency group by first checking if there
 * are any pending registration items for the same child and dependency group. If such an item exists, it is
 * removed from the pending items queue, effectively canceling the registration. If there is also a pending
 * dependency group state update triggered by the same child, it is either removed or updated to use a different
 * child if the group is not being removed entirely. If no pending registration exists, the function proceeds
 * to enqueue the necessary deletions in Redis for the dependencies and related nodes and edges.
 *
 * @param depGroup The dependency group from which the child is being removed.
 * @param dependencies The list of dependencies associated with the child being removed.
 * @param removeGroup A flag indicating whether the entire dependency group should be removed.
 */
void IcingaDB::EnqueueDependencyChildRemoved(
	const DependencyGroup::Ptr& depGroup,
	const std::vector<Dependency::Ptr>& dependencies,
	bool removeGroup
)
{
	if (dependencies.empty() || !GetActive() || !m_RconWorker || !m_RconWorker->IsConnected()) {
		return; // No need to enqueue anything if we're not connected or there are no dependencies.
	}

	Checkable::Ptr child(dependencies.front()->GetChild());
	bool hadPendingRegistration = false; // Whether we had a pending child registration to cancel.

	{
		std::lock_guard lock(m_PendingItemsMutex);
		if (auto it(m_PendingItems.find(std::make_pair(child, depGroup))); it != m_PendingItems.end()) {
			hadPendingRegistration = true;
			m_PendingItems.erase(it); // Cancel the pending child registration.
			if (removeGroup) {
				// If we're removing the entire group registration, we can also drop any pending dependency group
				// state update triggered previously as it should no longer have any children left.
				m_PendingItems.erase(std::make_pair(nullptr, depGroup));
			}
		}
	}

	if (!child->HasAnyDependencies()) {
		// If the child Checkable has no parent and reverse dependencies, we can safely remove the dependency node.
		// This might be a no-op in some cases (e.g. if the child's only dependency was the one that we just canceled
		// above), but since we can't reliably determine whether the node exists in Redis or not, we just enqueue the
		// deletion anyway.
		EnqueueRelationsDeletion(GetObjectIdentifier(child), {{RedisKey::DependencyNode, false}});
	}

	if (hadPendingRegistration && depGroup->GetIcingaDBIdentifier().IsEmpty()) {
		// If we had a pending registration that we just canceled above, and the dependency group has no
		// IcingaDB identifier yet, then there's no need to proceed with any deletions, as the dependency
		// group was never serialized to Redis in the first place.
		return;
	}

	if (depGroup->IsRedundancyGroup() && depGroup->GetIcingaDBIdentifier().IsEmpty()) {
		// An empty IcingaDB identifier indicates that the worker thread has just picked up the registration of the
		// first child (removed from the pending items queue) but hasn't yet entered the InsertCheckableDependencies()
		// function to actually fill in the IcingaDB identifier. Thus, we need to generate and set it here to ensure
		// that the relation deletions below use the correct identifier.
		//
		// Note: keep this with IcingaDB::InsertCheckableDependencies in sync!
		depGroup->SetIcingaDBIdentifier(HashValue(new Array{m_EnvironmentId, depGroup->GetCompositeKey()}));
	}

	std::set<Checkable*> detachedParents;
	for (const auto& dependency : dependencies) {
		const auto& parent(dependency->GetParent());
		if (auto [_, inserted] = detachedParents.insert(dependency->GetParent().get()); inserted) {
			String edgeId;
			if (depGroup->IsRedundancyGroup()) {
				// If the redundancy group has no members left, it's going to be removed as well, so we need to
				// delete dependency edges from that group to the parent Checkables.
				if (removeGroup) {
					EnqueueRelationsDeletion(
						GetDependencyEdgeStateId(depGroup, dependency),
						{
							{RedisKey::DependencyEdge, false},
							{RedisKey::DependencyEdgeState, false},
						}
					);
				}

				// Remove the connection from the child Checkable to the redundancy group.
				edgeId = HashValue(new Array{GetObjectIdentifier(child), depGroup->GetIcingaDBIdentifier()});
			} else {
				// Remove the edge between the parent and child Checkable linked through the removed dependency.
				edgeId = HashValue(new Array{GetObjectIdentifier(child), GetObjectIdentifier(parent)});
				if (depGroup->GetIcingaDBIdentifier().IsEmpty()) {
					(void)GetDependencyEdgeStateId(depGroup, dependency);
				}
			}

			EnqueueRelationsDeletion(std::move(edgeId), {{RedisKey::DependencyEdge, false}});

			// The total_children and affects_children columns might now have different outcome, so update the parent
			// Checkable as well. The grandparent Checkable may still have wrong numbers of total children, though it's
			// not worth traversing the whole tree way up and sending config updates for each one of them, as the next
			// Redis config dump is going to fix it anyway.
			EnqueueConfigObject(parent, ConfigUpdate);

			if (!parent->HasAnyDependencies()) {
				// If the parent Checkable isn't part of any other dependency chain anymore, drop its dependency node entry.
				EnqueueRelationsDeletion(GetObjectIdentifier(parent), {{RedisKey::DependencyNode, false}});
			}
		}
	}

	if (removeGroup && depGroup->IsRedundancyGroup()) {
		EnqueueRelationsDeletion(
			depGroup->GetIcingaDBIdentifier(),
			{
				{RedisKey::DependencyNode, false},
				{RedisKey::RedundancyGroup, false},
				{RedisKey::RedundancyGroupState, false},
				{RedisKey::DependencyEdgeState, false}
			}
		);
	} else if (removeGroup) {
		// Note: The Icinga DB identifier of a non-redundant dependency group is used as the edge state ID
		// and shared by all of its dependency objects. See also SerializeDependencyEdgeState() for details.
		EnqueueRelationsDeletion(depGroup->GetIcingaDBIdentifier(), {{RedisKey::DependencyEdgeState, false}});
	}
}

/**
 * Enqueue a relation deletion for processing in the pending objects thread.
 *
 * This function adds a relation deletion item to the set of pending items to be processed by the
 * pending items worker thread. The relation deletion item contains the ID of the relation to be
 * deleted and a map of Redis keys from which to delete the relation. If the relation deletion item
 * is already in the set, it updates the deletion keys accordingly.
 *
 * @param id The ID of the relation to be deleted.
 * @param relations A map of Redis keys from which to delete the relation.
 */
void IcingaDB::EnqueueRelationsDeletion(const String& id, const RelationsKeyMap& relations)
{
	if (!GetActive() || !m_RconWorker || !m_RconWorker->IsConnected()) {
		return; // No need to enqueue anything if we're not connected.
	}

	{
		std::lock_guard lock(m_PendingItemsMutex);
		if (auto [it, inserted] = m_PendingItems.insert(RelationsDeletionItem{id, relations}); !inserted) {
			m_PendingItems.modify(it, [&relations](PendingItemVariant& val) {
				auto& item = std::get<RelationsDeletionItem>(val);
				item.Relations.insert(relations.begin(), relations.end());
			});
		}
	}
	m_PendingItemsCV.notify_one();
}
