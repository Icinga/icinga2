// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "icingadb/icingadb.hpp"
#include "base/logger.hpp"
#include <vector>

using namespace icinga;

icingadb::task_queue::PendingConfigItem::PendingConfigItem(const ConfigObject::Ptr& obj, uint32_t bits)
	: Object{obj}, DirtyBits{bits & DirtyBitsAll}
{
}

icingadb::task_queue::PendingDependencyGroupStateItem::PendingDependencyGroupStateItem(const DependencyGroup::Ptr& depGroup)
	: DepGroup{depGroup}
{
}

icingadb::task_queue::PendingDependencyEdgeItem::PendingDependencyEdgeItem(const DependencyGroup::Ptr& depGroup, const Checkable::Ptr& child)
	: DepGroup{depGroup}, Child{child}
{
}

icingadb::task_queue::RelationsDeletionItem::RelationsDeletionItem(const String& id, const RelationsKeySet& relations)
	: ID{id}, Relations{relations}
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
	constexpr std::size_t maxPendingQueries = 128;

	std::unique_lock lock(m_PendingItemsMutex);
	// Wait until the initial config dump is done. IcingaDB::OnConnectedHandler will notify us once it's finished.
	while (GetActive() && !m_ConfigDumpDone) m_PendingItemsCV.wait(lock);

	while (GetActive()) {
		if (!m_PendingItems.empty() && m_RconWorker && m_RconWorker->IsConnected() && m_RconWorker->GetPendingQueryCount() < maxPendingQueries) {
			if (auto retryAfter = DequeueAndProcessOne(lock); retryAfter > 0ms) {
				m_PendingItemsCV.wait_for(lock, retryAfter);
			}
		} else {
			// In case we don't receive any notification, we still want to wake up periodically on our own
			// to check whether we can proceed (e.g. the Redis connection might have become available again and there
			// was no activity on the pending items queue to trigger a notification). Thus, we use a timed wait here.
			m_PendingItemsCV.wait_for(lock, 100ms);
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
	namespace queue = icingadb::task_queue;

	bool madeProgress = false; // Did we make any progress in this iteration?
	ch::duration<double> retryAfter{0}; // If we can't process anything right now, how long to wait before retrying?
	auto now = ch::steady_clock::now();

	auto& seqView = m_PendingItems.get<1>();
	for (auto it(seqView.begin()); it != seqView.end(); ++it) {
		if (it != seqView.begin()) {
			if (std::holds_alternative<queue::RelationsDeletionItem>(it->Item)) {
				// We don't know whether the previous items are related to this deletion item or not,
				// thus we can't just process this right now when there are older items in the queue.
				// Otherwise, we might delete something that is going to be updated/created.
				break;
			}
		}

		if (auto age = now - it->EnqueueTime; 1000ms > age) {
			if (it == seqView.begin()) {
				retryAfter = 1000ms - age;
			}
			break;
		}

		ConfigObject::Ptr cobj;
		if (auto confPtr = std::get_if<queue::PendingConfigItem>(&it->Item); confPtr) {
			cobj = confPtr->Object;
		} else if (auto edgePtr = std::get_if<queue::PendingDependencyEdgeItem>(&it->Item)) {
			cobj = edgePtr->Child;
		}
		ObjectLock olock(cobj, std::defer_lock);
		if (cobj && !olock.TryLock()) {
			continue; // Can't lock the object right now, try the next one.
		}

		auto itemToProcess = *it;
		seqView.erase(it);
		madeProgress = true;

		lock.unlock();
		std::visit([this](auto &item) {
			try {
				ProcessQueueItem(item);
			} catch (const std::exception& ex) {
				Log(LogCritical, "IcingaDB")
					<< "Exception while processing pending item of type '" << typeid(decltype(item)).name() << "': "
					<< DiagnosticInformation(ex, GetActive());
			}
		}, itemToProcess.Item);
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
 * Execute the pending configuration item.
 *
 * This function processes the pending configuration item by performing the necessary Redis operations based
 * on the dirty bits set for the associated configuration object. It handles configuration deletions, updates,
 * and state updates for checkable objects.
 *
 * @param item The queue item to process.
 */
void IcingaDB::ProcessQueueItem(const icingadb::task_queue::PendingConfigItem& item)
{
	namespace queue = icingadb::task_queue;

	if (item.DirtyBits & queue::ConfigDelete) {
		auto redisKeyPair = GetSyncableTypeRedisKeys(item.Object->GetReflectionType());
		m_RconWorker->FireAndForgetQueries(
			{
				{"HDEL", redisKeyPair.ObjectKey, GetObjectIdentifier(item.Object)},
				{"HDEL", redisKeyPair.ChecksumKey, GetObjectIdentifier(item.Object)},
				{
					"XADD",
					"icinga:runtime",
					"MAXLEN",
					"~",
					"1000000",
					"*",
					"redis_key",
					redisKeyPair.ObjectKey,
					"id",
					GetObjectIdentifier(item.Object),
					"runtime_type",
					"delete"
				}
			}
		);
	}

	if (item.DirtyBits & queue::ConfigUpdate) {
		std::map<RedisConnection::QueryArg, RedisConnection::Query> hMSets;
		std::vector<Dictionary::Ptr> runtimeUpdates;
		CreateConfigUpdate(item.Object, GetSyncableTypeRedisKeys(item.Object->GetReflectionType()), hMSets, runtimeUpdates, true);
		ExecuteRedisTransaction(m_RconWorker, hMSets, runtimeUpdates);
	}

	if (auto checkable = dynamic_pointer_cast<Checkable>(item.Object); checkable) {
		if (item.DirtyBits & queue::FullState) {
			UpdateState(checkable, item.DirtyBits);
		}
		if (item.DirtyBits & queue::NextUpdate) {
			SendNextUpdate(checkable);
		}
	}
}

/**
 * Execute the pending dependency group state item.
 *
 * This function processes the pending dependency group state item by updating the state of the
 * dependency group in Redis. It selects any child checkable from the dependency group to initiate
 * the state update, as all children share the same dependency group state.
 *
 * @param item The queue item to process.
 */
void IcingaDB::ProcessQueueItem(const icingadb::task_queue::PendingDependencyGroupStateItem& item) const
{
	// For dependency group state updates, we don't actually care which child triggered the update,
	// since all children share the same dependency group state. Thus, we can just pick any child to
	// start the update from.
	if (auto child = item.DepGroup->GetAnyChild(); child) {
		UpdateDependenciesState(child, item.DepGroup);
	}
}

/**
 * Execute the pending dependency edge item.
 *
 * This function processes the pending dependency edge item and ensures that the necessary Redis
 * operations are performed to register the child checkable as part of the dependency group.
 *
 * @param item The queue item to process.
 */
void IcingaDB::ProcessQueueItem(const icingadb::task_queue::PendingDependencyEdgeItem& item)
{
	std::vector<Dictionary::Ptr> runtimeUpdates;
	std::map<RedisConnection::QueryArg, RedisConnection::Query> hMSets;
	InsertCheckableDependencies(item.Child, hMSets, &runtimeUpdates, item.DepGroup);
	ExecuteRedisTransaction(m_RconWorker, hMSets, runtimeUpdates);
}

/**
 * Execute the pending relations deletion item.
 *
 * This function processes the pending relations deletion item by deleting the specified relations
 * from Redis. It iterates over the map of Redis keys and deletes the relations associated with
 * the given ID.
 *
 * @param item The queue item to process.
 */
void IcingaDB::ProcessQueueItem(const icingadb::task_queue::RelationsDeletionItem& item)
{
	for (const auto& [configKey, checksumKey] : item.Relations) {
		if (IsStateKey(configKey)) {
			DeleteState(item.ID, configKey, checksumKey);
		} else {
			DeleteRelationship(item.ID, configKey, checksumKey);
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
	namespace queue = icingadb::task_queue;

	if (!GetActive() || !m_RconWorker || !m_RconWorker->IsConnected()) {
		return; // No need to enqueue anything if we're not connected.
	}

	{
		std::lock_guard lock(m_PendingItemsMutex);
		if (auto [it, inserted] = m_PendingItems.emplace(queue::PendingConfigItem{object, bits}); !inserted) {
			m_PendingItems.modify(it, [bits](queue::PendingQueueItem& item) {
				auto& configItem = std::get<queue::PendingConfigItem>(item.Item);
				if (bits & queue::ConfigDelete) {
					configItem.DirtyBits &= ~(queue::ConfigUpdate | queue::FullState);
				} else if (bits & queue::ConfigUpdate) {
					configItem.DirtyBits &= ~queue::ConfigDelete;
				}
				configItem.DirtyBits |= bits & queue::DirtyBitsAll;
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
			m_PendingItems.emplace(icingadb::task_queue::PendingDependencyGroupStateItem{depGroup});
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
			m_PendingItems.emplace(icingadb::task_queue::PendingDependencyEdgeItem{depGroup, child});
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
	namespace queue = icingadb::task_queue;

	if (dependencies.empty() || !GetActive() || !m_RconWorker || !m_RconWorker->IsConnected()) {
		return; // No need to enqueue anything if we're not connected or there are no dependencies.
	}

	Checkable::Ptr child(dependencies.front()->GetChild());
	bool cancelledRegistration = false;

	{
		std::lock_guard lock(m_PendingItemsMutex);
		if (m_PendingItems.erase(std::make_pair(depGroup.get(), child.get())) > 0) {
			cancelledRegistration = true;
			if (removeGroup) {
				// If we're removing the entire group registration, we can also drop any pending dependency group
				// state update triggered previously as it should no longer have any children left.
				m_PendingItems.erase(depGroup.get());
			}
		}
	}

	if (!child->HasAnyDependencies()) {
		// If the child Checkable has no parent and reverse dependencies, we can safely remove the dependency node.
		// This might be a no-op in some cases (e.g. if the child's only dependency was the one that we just canceled
		// above), but since we can't reliably determine whether the node exists in Redis or not, we just enqueue the
		// deletion anyway.
		EnqueueRelationsDeletion(GetObjectIdentifier(child), {{CONFIG_REDIS_KEY_PREFIX "dependency:node", ""}});
	}

	if (cancelledRegistration && depGroup->GetIcingaDBIdentifier().IsEmpty()) {
		// If we had a pending registration that we just canceled above, and the dependency group has no
		// IcingaDB identifier yet, then there's no need to proceed with any deletions, as the dependency
		// group was never serialized to Redis in the first place.
		return;
	}

	if (depGroup->GetIcingaDBIdentifier().IsEmpty()) {
		// An empty IcingaDB identifier indicates that the worker thread has just picked up the registration of the
		// first child (removed from the pending items queue) but hasn't yet entered the InsertCheckableDependencies()
		// function to actually fill in the IcingaDB identifier. Thus, we need to generate and set it here to ensure
		// that the relation deletions below use the correct identifier.
		if (depGroup->IsRedundancyGroup()) {
			// Keep this with IcingaDB::InsertCheckableDependencies in sync!
			depGroup->SetIcingaDBIdentifier(HashValue(new Array{m_EnvironmentId, depGroup->GetCompositeKey()}));
		} else {
			// This will set the IcingaDB identifier of the dependency group as a side effect.
			(void)GetDependencyEdgeStateId(depGroup, dependencies.front());
		}
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
							{CONFIG_REDIS_KEY_PREFIX "dependency:edge", ""},
							{CONFIG_REDIS_KEY_PREFIX "dependency:edge:state", ""},
						}
					);
				}

				// Remove the connection from the child Checkable to the redundancy group.
				edgeId = HashValue(new Array{GetObjectIdentifier(child), depGroup->GetIcingaDBIdentifier()});
			} else {
				// Remove the edge between the parent and child Checkable linked through the removed dependency.
				edgeId = HashValue(new Array{GetObjectIdentifier(child), GetObjectIdentifier(parent)});
			}

			EnqueueRelationsDeletion(edgeId, {{CONFIG_REDIS_KEY_PREFIX "dependency:edge", ""}});

			// The total_children and affects_children columns might now have different outcome, so update the parent
			// Checkable as well. The grandparent Checkable may still have wrong numbers of total children, though it's
			// not worth traversing the whole tree way up and sending config updates for each one of them, as the next
			// Redis config dump is going to fix it anyway.
			EnqueueConfigObject(parent, queue::ConfigUpdate);

			if (!parent->HasAnyDependencies()) {
				// If the parent Checkable isn't part of any other dependency chain anymore, drop its dependency node entry.
				EnqueueRelationsDeletion(GetObjectIdentifier(parent), {{CONFIG_REDIS_KEY_PREFIX "dependency:node", ""}});
			}
		}
	}

	if (removeGroup && depGroup->IsRedundancyGroup()) {
		EnqueueRelationsDeletion(
			depGroup->GetIcingaDBIdentifier(),
			{
				{CONFIG_REDIS_KEY_PREFIX "dependency:node", ""},
				{CONFIG_REDIS_KEY_PREFIX "redundancygroup", ""},
				{CONFIG_REDIS_KEY_PREFIX "redundancygroup:state", ""},
				{CONFIG_REDIS_KEY_PREFIX "dependency:edge:state", ""}
			}
		);
	} else if (removeGroup) {
		// Note: The Icinga DB identifier of a non-redundant dependency group is used as the edge state ID
		// and shared by all of its dependency objects. See also SerializeDependencyEdgeState() for details.
		EnqueueRelationsDeletion(depGroup->GetIcingaDBIdentifier(), {{CONFIG_REDIS_KEY_PREFIX "dependency:edge:state", ""}});
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
void IcingaDB::EnqueueRelationsDeletion(const String& id, icingadb::task_queue::RelationsDeletionItem::RelationsKeySet relations)
{
	namespace queue = icingadb::task_queue;

	if (!GetActive() || !m_RconWorker || !m_RconWorker->IsConnected()) {
		return; // No need to enqueue anything if we're not connected.
	}

	{
		std::lock_guard lock(m_PendingItemsMutex);
		if (auto [it, inserted] = m_PendingItems.emplace(queue::RelationsDeletionItem{id, relations}); !inserted) {
			m_PendingItems.modify(it, [&relations](queue::PendingQueueItem& val) {
				auto& item = std::get<queue::RelationsDeletionItem>(val.Item);
				item.Relations.merge(std::move(relations));
			});
		}
	}
	m_PendingItemsCV.notify_one();
}
