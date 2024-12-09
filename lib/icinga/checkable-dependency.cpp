/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/logger.hpp"
#include "base/object-packer.hpp"
#include "base/utility.hpp"
#include "icinga/dependency.hpp"
#include "icinga/service.hpp"
#include <numeric>

using namespace icinga;

boost::signals2::signal<void (const Dependency::Ptr&, const std::vector<RedundancyGroup::Ptr>&)>Checkable::OnRedundancyGroupsChanged;

std::mutex RedundancyGroup::m_RegistryMutex;
RedundancyGroup::RegistryType RedundancyGroup::m_Registry;

// Is the default (dummy) redundancy group used to group non-redundant dependencies.
static String l_DefaultRedundancyGroup(Utility::NewUniqueID());

/**
 * Register a redundancy group in the global redundancy groups registry.
 *
 * At first, it tries to naively insert the redundancy group into the registry. In case there is already an
 * identical redundancy group in the registry, the insertion will just fail. In this case, it will move all
 * members of the provided redundancy group into the existing one and re-register the existing one recursively
 * till there is no redundancy group with identical members left and the insertion finally succeeds.
 *
 * Note: This is a helper function intended for internal use only, and you should acquire the global registry mutex
 * before calling this function.
 *
 * @param redundancyGroup The redundancy group to register.
 */
void RedundancyGroup::RegisterRedundancyGroup(const RedundancyGroup::Ptr& redundancyGroup)
{
	if (auto it(m_Registry.insert(redundancyGroup.get())); !it.second) {
		RedundancyGroup::Ptr existingGroup(*it.first);
		if (redundancyGroup->IsDefault()) {
			// It's the dummy group, so just move the new members into the existing one, and it should be it.
			redundancyGroup->MoveMembersTo(existingGroup);
		} else {
			// Erase it before we move the members into that group and change the hash.
			m_Registry.erase(it.first);
			redundancyGroup->MoveMembersTo(existingGroup);
			RegisterRedundancyGroup(existingGroup);
		}
	}
}

/**
 * Refresh the registry of redundancy groups.
 *
 * This function is used to refresh the global registry of redundancy groups.
 * It will try to find an existing redundancy group the child Checkable of the given dependency is member of,
 * and if found, it will move the remaining dependencies of the Checkable into the new redundancy group and add
 * it to the registry. A nullptr as a new redundancy group means we want to unregister the provided dependency,
 * thus it will not be moved into the temporary new redundancy group constructed within this function.
 *
 * Note: This is a helper function intended for internal use only, and you should acquire the global registry mutex
 * before calling this function.
 *
 * @param dependency The dependency object to refresh the registry for.
 * @param newGroup The new redundancy group to move the remaining Checkable dependencies into.
 */
void RedundancyGroup::RefreshRegistry(const Dependency::Ptr& dependency, const RedundancyGroup::Ptr& newGroup)
{
	auto [rangeBegin, rangeEnd] = m_Registry.get<1>().equal_range(dependency->GetRedundancyGroup());
	for (auto groupIt(rangeBegin); groupIt != rangeEnd; ++groupIt) {
		RedundancyGroup::Ptr existingGroup(*groupIt);
		if (existingGroup->HasMembers(dependency->GetChild())) {
			// Erase the existing redundancy group from the registry, before we move the members
			// out of it and change its identity, i.e. the hash value used by the registry.
			m_Registry.erase(existingGroup.get());

			RedundancyGroup::Ptr replacementGroup(newGroup);
			for (auto& [_, dependencies] : existingGroup->GetMembers()) {
				auto [begin, end] = dependencies.equal_range(dependency->GetChild().get());
				for (auto it(begin); it != end; ++it) {
					// A nullptr newGroup means we want to unregister the provided dependency object. Otherwise,
					// it should already be registered to the new redundancy group, and we just need to move the
					// remaining dependencies of the Checkable into that new group.
					if (newGroup || it->second != dependency) {
						if (!replacementGroup) {
							replacementGroup = new RedundancyGroup(existingGroup->GetName(), it->second);
						} else {
							// If there are any dependencies registered for the same child Checkable under the existing
							// redundancy group, we must move them into the new one. Meaning, the redundancy group for
							// that Checkable has changed, meanwhile for the other Checkables it's still the same.
							replacementGroup->AddMember(it->second);
						}
					}
					existingGroup->RemoveMember(it->second);
				}
			}

			if (existingGroup->HasMembers()) {
				// Detach the existing redundancy group from the child Checkable of the dependency
				// object, as it's not a member of it anymore. Instead, we must...
				dependency->GetChild()->RemoveRedundancyGroup(existingGroup);

				if (replacementGroup) {
					// ...attach the new redundancy group to the child Checkable.
					dependency->GetChild()->AddRedundancyGroup(replacementGroup);
					RegisterRedundancyGroup(replacementGroup);
				}

				// The existing redundancy group still has some members left, so we must re-register
				// it and enforce the rehashing of the members due to the removed ones above.
				RegisterRedundancyGroup(existingGroup);
			} else if (replacementGroup) {
				// We were the last member of the existing redundancy group, so instead of replacing it with the
				// replacement group, we can just move back the members to it and re-add it to the registry.
				replacementGroup->MoveMembersTo(existingGroup);
				RegisterRedundancyGroup(existingGroup);
			} else {
				// The existing redundancy group has no members left, so we must detach it from the child Checkable.
				dependency->GetChild()->RemoveRedundancyGroup(existingGroup);
			}

			return;
		}
	}

	if (newGroup) {
		// If we haven't found any existing redundancy group to register the dependency to, we must
		// attach the new redundancy group to the child Checkable and register the new group to the registry.
		dependency->GetChild()->AddRedundancyGroup(newGroup);
		RegisterRedundancyGroup(newGroup);
	}
}

/**
 * Register the provided dependency to the global redundancy group registry.
 *
 * @param dependency The dependency to register.
 */
void RedundancyGroup::Register(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_RegistryMutex);
	auto groupName(dependency->GetRedundancyGroup());
	RedundancyGroup::Ptr newGroup(new RedundancyGroup(groupName.IsEmpty() ? l_DefaultRedundancyGroup : groupName, dependency));
	if (m_Registry.empty() || groupName.IsEmpty()) {
		dependency->GetChild()->AddRedundancyGroup(newGroup);
		RegisterRedundancyGroup(newGroup);
		return;
	}

	RefreshRegistry(dependency, newGroup);
}

/**
 * Unregister the provided dependency from the redundancy group it was member of.
 *
 * @param dependency The dependency to unregister.
 */
void RedundancyGroup::Unregister(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_RegistryMutex);
	if (dependency->GetRedundancyGroup().IsEmpty()) {
		// Default redundancy group of a given Checkable always produce the very same hash, so we can
		// safely use any instance of it to find the existing one in the registry.
		RedundancyGroup::Ptr defaultGroup(new RedundancyGroup(l_DefaultRedundancyGroup, dependency));
		if (auto it(m_Registry.find(defaultGroup)); it != m_Registry.end()) {
			RedundancyGroup::Ptr existingGroup(*it);
			existingGroup->RemoveMember(dependency);
			if (!existingGroup->HasMembers()) {
				// There are no members left in the default redundancy group, so we must detach
				// it from the child Checkable of the dependency object.
				dependency->GetChild()->RemoveRedundancyGroup(existingGroup);
				m_Registry.erase(it);
			}
		}
		return;
	}

	RefreshRegistry(dependency, Ptr());
}

/**
 * Retrieve the size of the global redundancy group registry.
 *
 * @return size_t - Returns the size of the global redundancy group registry.
 */
size_t RedundancyGroup::GetRegistrySize()
{
	std::lock_guard lock(m_RegistryMutex);
	return m_Registry.size();
}

RedundancyGroup::RedundancyGroup(String name, const Dependency::Ptr& dependency): m_Name(std::move(name))
{
	AddMember(dependency);
}

/**
 * Check if the provided redundancy group matches the randomly generated (default) one.
 *
 * @param name The redundancy group name you want to check.
 *
 * @return bool - Returns true if the provided redundancy group matches the randomly generated (default) one.
 */
bool RedundancyGroup::IsDefault(const String& name)
{
	return  name == l_DefaultRedundancyGroup;
}

/**
 * Create a composite key for the provided dependency.
 *
 * @param dep The dependency object to create a composite key for.
 *
 * @return - Returns the composite key for the provided dependency.
 */
RedundancyGroup::MemberTuple RedundancyGroup::MakeCompositeKeyFor(const Dependency::Ptr& dep)
{
	if (dep->GetRedundancyGroup().IsEmpty()) {
		// Just to make sure we don't have any duplicates in the default group, i.e. each dependency object
		// should be produce different hash value within the group itself. Note, this isn't used to determine
		// the identity of the redundancy group, but to batch the individual group members.
		return std::make_tuple(l_DefaultRedundancyGroup, dep->GetChild()->GetName(), 0, false);
	}

	return std::make_tuple(
		dep->GetParent()->GetName(),
		dep->GetPeriodRaw(),
		dep->GetStateFilter(),
		dep->GetIgnoreSoftStates()
	);
}

/**
 * Check if the current redundancy group is the default one.
 *
 * @return bool - Returns true if the current redundancy group is the default one.
 */
bool RedundancyGroup::IsDefault() const
{
    return IsDefault(m_Name);
}

/**
 * Check if the current redundancy group has any members.
 *
 * @return bool - Returns true if the current redundancy group has any members.
 */
bool RedundancyGroup::HasMembers() const
{
	std::lock_guard lock(m_Mutex);
	return !m_Members.empty();
}

/**
 * Check if the current redundancy group has any members the provided child Checkable depend on.
 *
 * @param child The child Checkable to look for.
 *
 * @return bool - Returns true if the current redundancy group has any members the provided child depend on.
 */
bool RedundancyGroup::HasMembers(const Checkable::Ptr& child) const
{
	std::lock_guard lock(m_Mutex);
	return std::any_of(m_Members.begin(), m_Members.end(), [child](const auto& member) {
		return member.second.find(child.get()) != member.second.end();
	});
}

/**
 * Retrieve a copy of the members of the current redundancy group.
 *
 * @return - Returns all the members of the current redundancy group.
 */
RedundancyGroup::MembersMap RedundancyGroup::GetMembers() const
{
	std::lock_guard lock(m_Mutex);
	return {m_Members.begin(), m_Members.end()};
}

/**
 * Retrieve all members of the current redundancy group the provided child Checkable depend on.
 *
 * @param child The child Checkable to look for.
 *
 * @return - Returns all members of the current redundancy group the provided child depend on.
 */
std::set<Dependency::Ptr> RedundancyGroup::GetMembers(const Checkable* child) const
{
	std::lock_guard lock(m_Mutex);
	std::set<Dependency::Ptr> members;
	for (auto& [_, dependencies] : m_Members) {
		auto range(dependencies.equal_range(child));
		std::transform(range.first,range.second, std::inserter(members, members.end()), [](const auto& member) {
			return member.second;
		});
	}

	return members;
}

/**
 * Retrieve the number of members in the current redundancy group.
 *
 * This function mainly exists for optimization purposes, i.e. instead of getting a copy of the members and
 * counting them, we can directly query the number of members.
 *
 * @return - Returns the number of members in the current redundancy group.
 */
size_t RedundancyGroup::GetMemberCount() const
{
	std::lock_guard lock(m_Mutex);
	return std::accumulate(m_Members.begin(), m_Members.end(), static_cast<size_t>(0), [](int sum, const auto& pair) {
		return sum + pair.second.size();
	});
}

/**
 * Add a member to the current redundancy group.
 *
 * @param member The dependency to add to the redundancy group.
 */
void RedundancyGroup::AddMember(const Dependency::Ptr& member)
{
	std::lock_guard lock(m_Mutex);
	MemberTuple compositeKey(MakeCompositeKeyFor(member));
	if (auto it(m_Members.find(compositeKey)); it != m_Members.end()) {
		it->second.emplace(member->GetChild().get(), member.get());
	} else {
		m_Members.emplace(compositeKey, MemberValueType{{member->GetChild().get(), member.get()}});
	}
}

/**
 * Remove a member from the current redundancy group.
 *
 * @param member The dependency to remove from the redundancy group.
 */
void RedundancyGroup::RemoveMember(const Dependency::Ptr& member)
{
	std::lock_guard lock(m_Mutex);
	if (auto it(m_Members.find(MakeCompositeKeyFor(member))); it != m_Members.end()) {
		auto [rangeBegin, rangeEnd] = it->second.equal_range(member->GetChild().get());
		for (auto memberIt(rangeBegin); memberIt != rangeEnd; ++memberIt) {
			if (memberIt->second == member) {
				// This will also remove the child Checkable from the multimap container
				// entirely if this was the last member of it.
				it->second.erase(memberIt);
				// If the composite key has no more members left, we can remove it entirely as well.
				if (it->second.empty()) {
					m_Members.erase(it);
				}
				return;
			}
		}
	}
}

/**
 * Move the members of the provided redundancy group to the provided destination redundancy group.
 *
 * @param dest The redundancy group to move the members to.
 */
void RedundancyGroup::MoveMembersTo(const RedundancyGroup::Ptr& dest)
{
	std::lock_guard lock(m_Mutex);
	RedundancyGroup::Ptr thisPtr(this); // Just in case the Checkable below was our last reference.
	for (auto& [_, members] : m_Members) {
		Checkable::Ptr previousChild;
		for (auto& [checkable, dependency] : members) {
			dest->AddMember(dependency);
			if (!previousChild || previousChild != checkable) {
				previousChild = dependency->GetChild();
				previousChild->RemoveRedundancyGroup(thisPtr);
				previousChild->AddRedundancyGroup(dest);
			}
		}
	}
}

/**
 * Set the Icinga DB identifier for the current redundancy group.
 *
 * The only usage of this function is the Icinga DB feature used to cache the unique hash of this redundancy groups.
 *
 * @param identifier The Icinga DB identifier to set.
 */
void RedundancyGroup::SetIcingaDBIdentifier(const String& identifier)
{
	std::lock_guard lock(m_Mutex);
	m_IcingaDBIdentifier = identifier;
}

/**
 * Retrieve the Icinga DB identifier for the current redundancy group.
 *
 * When the identifier is not already set by Icinga DB via the SetIcingaDBIdentifier method,
 * this will just return an empty string.
 *
 * @return - Returns the Icinga DB identifier for the current redundancy group.
 */
String RedundancyGroup::GetIcingaDBIdentifier() const
{
	std::lock_guard lock(m_Mutex);
	return m_IcingaDBIdentifier;
}

/**
 * Retrieve the (non-unique) name of the current redundancy group.
 *
 * The name of the redundancy group is the same as the one located in the configuration files.
 *
 * @return - Returns the name of the current redundancy group.
 */
const String& RedundancyGroup::GetName() const
{
	// We don't need to lock the mutex here, as the name is set once during
	// the object construction and never changed afterwards.
	return m_Name;
}

/**
 * Retrieve the unique composite key of the current redundancy group.
 *
 * The composite key consists of some unique data of the redundancy group members, and should be used
 * to generate a unique deterministic hash for the redundancy group. Each key is a tuple of the parent name,
 * the time period name (empty if not configured), the state filter, and the ignore soft states flag of the member.
 *
 * Additionally, to all the above mentioned keys, the non-unique redundancy group name is also included.
 *
 * @return - Returns the composite key of the current redundancy group.
 */
String RedundancyGroup::GetCompositeKey() const
{
	std::lock_guard lock(m_Mutex);
	if (IsDefault()) {
		// The default redundancy group is a special case, as it groups all non-redundant dependencies.
		// Thus, packing the name of the dummy group and child Checkable name is always unique enough.
		String childCheckableName;
		if (!m_Members.empty()) {
			auto [_, members] = *m_Members.begin();
			childCheckableName = members.begin()->first->GetName();
		}
		return PackObject(new Array{GetName(), childCheckableName, 0, false});
	}

	Array::Ptr data(new Array{GetName()});
	std::for_each(m_Members.begin(), m_Members.end(), [&data](const auto& member) {
		auto [parentName, tpName, stateFilter, ignoreSoftStates] = member.first;
		data->Add(std::move(parentName));
		data->Add(std::move(tpName));
		data->Add(stateFilter);
		data->Add(ignoreSoftStates);
	});

	return PackObject(data);
}

/**
 * Retrieve the state of the current redundancy group.
 *
 * The state of the redundancy group is determined based on the state of the members of the group.
 * This method returns a RedundancyGroup::State::Unknown immediately when the redundancy group has no members.
 * Otherwise, a redundancy group is considered unreachable when any of the members is unreachable.
 * A reachable redundancy group is failed when the edges connected to it are not available.
 *
 * @return - Returns the state of the current redundancy group.
 */
RedundancyGroup::State RedundancyGroup::GetState(DependencyType dt) const
{
	if (!HasMembers()) {
		return State::Unknown;
	}

	bool isDefaultGroup(IsDefault());
	State state(State::Failed);
	// We don't want to hold the mutex lock for the entire evaluation, thus we just need to operate on a copy.
	MembersMap members(GetMembers());
	for (auto it(members.begin()); it != members.end() && (isDefaultGroup || state != State::ReachableAndOK); ++it) {
		// Those are first batch of group members, i.e. they all share kind of the same Dependency config.
		// So, for non-default groups, we only need to check a single parent Checkable of each such a group.
		for (auto& [checkable, dependency] : it->second) {
			if (!dependency->GetParent()->IsReachable(dt)) {
				if (isDefaultGroup) {
					// If any of the members is unreachable, the whole redundancy group is unreachable, too.
					return State::Unreachable;
				}
				state = State::UnreachableFailed;
				break;
			}

			if (!dependency->IsAvailable(dt)) {
				if (isDefaultGroup) {
					Log(LogDebug, "Checkable")
						<< "Non-redundant dependency '" << dependency->GetName() << "' failed for checkable '"
						<< checkable->GetName() << "': Marking as unreachable.";

					return State::Failed;
				}
				break;
			}

			state = State::ReachableAndOK;
			if (!isDefaultGroup) {
				break;
			}
		}
	}

	return state;
}

void Checkable::AddDependency(const Dependency::Ptr& dep) const
{
	std::vector<RedundancyGroup::Ptr> oldGroups;
	if (!dep->GetRedundancyGroup().IsEmpty()) {
		oldGroups = GetRedundancyGroups();
	}

	RedundancyGroup::Register(dep);
	Checkable::OnRedundancyGroupsChanged(dep, oldGroups);
}

void Checkable::AddRedundancyGroup(const RedundancyGroup::Ptr& redundancyGroup)
{
	std::unique_lock lock(m_DependencyMutex);
	m_Dependencies.emplace(redundancyGroup);
}

void Checkable::RemoveDependency(const Dependency::Ptr& dep) const
{
	std::vector<RedundancyGroup::Ptr> oldGroups;
	if (!dep->GetRedundancyGroup().IsEmpty()) {
		oldGroups = GetRedundancyGroups();
	}

	RedundancyGroup::Unregister(dep);
	// We don't want to spam Icinga DB with false deletion events on every config reload, so trigger
	// the event only when the dependency object is really going to be deleted.
	if (!dep->IsActive() && dep->GetExtension("ConfigObjectDeleted")) {
		Checkable::OnRedundancyGroupsChanged(dep, oldGroups);
	}
}

void Checkable::RemoveRedundancyGroup(const RedundancyGroup::Ptr& redundancyGroup)
{
	std::unique_lock lock(m_DependencyMutex);
	m_Dependencies.erase(redundancyGroup);
}

std::vector<Dependency::Ptr> Checkable::GetDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	std::vector<Dependency::Ptr> dependencies;
	for (const auto& redundancyGroup : m_Dependencies) {
		auto members(redundancyGroup->GetMembers(this));
		dependencies.insert(dependencies.end(), members.begin(), members.end());
	}

	return dependencies;
}

std::vector<RedundancyGroup::Ptr> Checkable::GetRedundancyGroups() const {
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	return {m_Dependencies.begin(), m_Dependencies.end()};
}

void Checkable::AddReverseDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	m_ReverseDependencies.insert(dep);
}

void Checkable::RemoveReverseDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	m_ReverseDependencies.erase(dep);
}

std::vector<Dependency::Ptr> Checkable::GetReverseDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	return std::vector<Dependency::Ptr>(m_ReverseDependencies.begin(), m_ReverseDependencies.end());
}

bool Checkable::IsReachable(DependencyType dt, Dependency::Ptr *failedDependency, int rstack) const
{
	/* Anything greater than 256 causes recursion bus errors. */
	int limit = 256;

	if (rstack > limit) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << limit << ") for checkable '" << GetName() << "': Dependency failed.";

		return false;
	}

	for (const Checkable::Ptr& checkable : GetParents()) {
		if (!checkable->IsReachable(dt, failedDependency, rstack + 1))
			return false;
	}

	/* implicit dependency on host if this is a service */
	const auto *service = dynamic_cast<const Service *>(this);
	if (service && (dt == DependencyState || dt == DependencyNotification)) {
		Host::Ptr host = service->GetHost();

		if (host && host->GetState() != HostUp && host->GetStateType() == StateTypeHard) {
			if (failedDependency)
				*failedDependency = nullptr;

			return false;
		}
	}

	auto deps = GetDependencies();

	std::unordered_map<std::string, Dependency::Ptr> violated; // key: redundancy group, value: nullptr if satisfied, violating dependency otherwise

	for (const Dependency::Ptr& dep : deps) {
		std::string redundancy_group = dep->GetRedundancyGroup();

		if (!dep->IsAvailable(dt)) {
			if (redundancy_group.empty()) {
				Log(LogDebug, "Checkable")
					<< "Non-redundant dependency '" << dep->GetName() << "' failed for checkable '" << GetName() << "': Marking as unreachable.";

				if (failedDependency)
					*failedDependency = dep;

				return false;
			}

			// tentatively mark this dependency group as failed unless it is already marked;
			//  so it either passed before (don't overwrite) or already failed (so don't care)
			// note that std::unordered_map::insert() will not overwrite an existing entry
			violated.insert(std::make_pair(redundancy_group, dep));
		} else if (!redundancy_group.empty()) {
			violated[redundancy_group] = nullptr;
		}
	}

	auto violator = std::find_if(violated.begin(), violated.end(), [](auto& v) { return v.second != nullptr; });
	if (violator != violated.end()) {
		Log(LogDebug, "Checkable")
			<< "All dependencies in redundancy group '" << violator->first << "' have failed for checkable '" << GetName() << "': Marking as unreachable.";

		if (failedDependency)
			*failedDependency = violator->second;

		return false;
	}

	if (failedDependency)
		*failedDependency = nullptr;

	return true;
}

/**
 * Checks whether the last check result of this Checkable affects its child dependencies.
 *
 * @return bool - Returns true if the Checkable affects its child dependencies, otherwise false.
 */
bool Checkable::AffectsChildren() const
{
	auto cr(GetLastCheckResult());
	if (!cr || IsStateOK(cr->GetState()) || !IsReachable()) {
		// If there is no check result, the state is OK, or the Checkable is not reachable, we can't
		// safely determine whether the Checkable affects its child dependencies.
		return false;
	}

	for (auto& dep: GetReverseDependencies()) {
		if (!dep->IsAvailable(DependencyState)) {
			// If one of the child dependency is not available, then it's definitely due to the
			// current Checkable state, so we don't need to verify the remaining ones.
			return true;
		}
	}

	return false;
}

std::set<Checkable::Ptr> Checkable::GetParents() const
{
	std::set<Checkable::Ptr> parents;

	for (const Dependency::Ptr& dep : GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (parent && parent.get() != this)
			parents.insert(parent);
	}

	return parents;
}

std::set<Checkable::Ptr> Checkable::GetChildren() const
{
	std::set<Checkable::Ptr> parents;

	for (const Dependency::Ptr& dep : GetReverseDependencies()) {
		Checkable::Ptr service = dep->GetChild();

		if (service && service.get() != this)
			parents.insert(service);
	}

	return parents;
}

/**
 * Retrieve the total number of all the children of the current Checkable.
 *
 * Note, this function performs a recursive call chain traversing all the children of the current Checkable
 * up to a certain limit (256). When that limit is reached, it will log a warning message and abort the operation.
 * Thus, the returned number may not reflect the actual total number of children involved in the dependency chain.
 *
 * In fact, there is already a method GetAllChildren() that does kind of same thing, but it involves too much
 * overhead, copying and passing around sets, which is not necessary for this simple operation (just count them).
 *
 * @return int - Returns the total number of all the children of the current Checkable.
 */
int Checkable::GetAllChildrenCount(int rstack) const
{
	if (rstack > 256) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << 256 << ") for checkable '" << GetName() << "': aborting count.";

		// The limit from GetAllChildrenInternal() doesn't seem to make sense, and appears to be
		// some random number. So, this limit is set to 256 to match the limit in IsReachable().
		return 0;
	}

	int count = 0;
	// Actually, incrementing the rstack should be done once outside the loop here, but since IsReachable()
	// applies the limit the exact same way, it should be fine to limit the sum of calls to 256 here as well.
	for (auto& dependency: GetReverseDependencies()) {
		count += dependency->GetChild()->GetAllChildrenCount(rstack + 1) + 1;
	}

	return count;
}

std::set<Checkable::Ptr> Checkable::GetAllChildren() const
{
	std::set<Checkable::Ptr> children = GetChildren();

	GetAllChildrenInternal(children, 0);

	return children;
}

void Checkable::GetAllChildrenInternal(std::set<Checkable::Ptr>& children, int level) const
{
	if (level > 32)
		return;

	std::set<Checkable::Ptr> localChildren;

	for (const Checkable::Ptr& checkable : children) {
		std::set<Checkable::Ptr> cChildren = checkable->GetChildren();

		if (!cChildren.empty()) {
			GetAllChildrenInternal(cChildren, level + 1);
			localChildren.insert(cChildren.begin(), cChildren.end());
		}

		localChildren.insert(checkable);
	}

	children.insert(localChildren.begin(), localChildren.end());
}
