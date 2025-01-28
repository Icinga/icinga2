/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "base/object-packer.hpp"
#include <numeric>

using namespace icinga;

boost::signals2::signal<void (const Dependency::Ptr&, const std::vector<DependencyGroup::Ptr>&)> DependencyGroup::OnMembersChanged;

std::mutex DependencyGroup::m_RegistryMutex;
DependencyGroup::RegistryType DependencyGroup::m_Registry;

/**
 * Register the provided dependency group to the global dependency group registry.
 *
 * @param dependencyGroup The dependency group to register.
 */
DependencyGroup::Ptr DependencyGroup::Register(const DependencyGroup::Ptr& dependencyGroup)
{
	std::lock_guard lock(m_RegistryMutex);
	if (auto [it, inserted] = m_Registry.insert(dependencyGroup); !inserted) {
		dependencyGroup->MoveMembersTo(*it);
		return *it;
	}
	return dependencyGroup;
}

/**
 * Unregister the provided dependency group from the global dependency group registry.
 *
 * Note, the dependency group will be unregistered if and only if the provided child Checkable is the only member
 * of it, i.e. no other Checkable depend on that group. Otherwise, the unregister operation will be ignored.
 *
 * @param dependencyGroup The dependency group you want to unregister.
 * @param child The child Checkable to restrict the unregister operation to.
 *
 * @return bool - Returns true if the provided dependency group has been successfully unregistered.
 */
bool DependencyGroup::Unregister(const DependencyGroup::Ptr& dependencyGroup, const Checkable::Ptr& child)
{
	std::lock_guard lock(m_RegistryMutex);
	if (auto it(m_Registry.find(dependencyGroup)); it != m_Registry.end()) {
		auto existingGroup(*it);
		// Make sure to never unregister a dependency group that is still referenced by another Checkables.
		// Meaning, if the total number of members in the group is equal to the number of members the child Checkable
		// depend on, then that child is the only member of the group, and we can safely unregister the group.
		// Otherwise, just ignore the unregister request!
		if (auto members(existingGroup->GetMembers(child.get())); members.size() == existingGroup->GetMemberCount()) {
			m_Registry.erase(it);
			return true;
		}
	}
	return false;
}

/**
 * Retrieve the size of the global dependency group registry.
 *
 * @return size_t - Returns the size of the global dependency groups registry.
 */
size_t DependencyGroup::GetRegistrySize()
{
	std::lock_guard lock(m_RegistryMutex);
	return m_Registry.size();
}

DependencyGroup::DependencyGroup(String name, const Dependency::Ptr& dependency): m_Name(std::move(name))
{
	AddMember(dependency);
}

/**
 * Create a composite key for the provided dependency.
 *
 * @param dependency The dependency object to create a composite key for.
 *
 * @return - Returns the composite key for the provided dependency.
 */
DependencyGroup::CompositeKeyType DependencyGroup::MakeCompositeKeyFor(const Dependency::Ptr& dependency)
{
	return std::make_tuple(
		dependency->GetParent()->GetName(),
		dependency->GetPeriodRaw(),
		dependency->GetStateFilter(),
		dependency->GetIgnoreSoftStates()
	);
}

/**
 * Check if the current dependency group has any members.
 *
 * @return bool - Returns true if the current dependency group has any members.
 */
bool DependencyGroup::HasMembers() const
{
	std::lock_guard lock(m_Mutex);
	return !m_Members.empty();
}

/**
 * Check whether an identical dependency object is member of the current dependency group.
 *
 * Identical means any dependency object that produces the exact same key (CompositeKeyType) as the provided one.
 *
 * @param dependency The dependency to look for.
 *
 * @return bool - Returns true if the provided dependency is member of the current dependency group.
 */
bool DependencyGroup::HasIdenticalMember(const Dependency::Ptr& dependency) const
{
	std::lock_guard lock(m_Mutex);
	return m_Members.find(MakeCompositeKeyFor(dependency)) != m_Members.end();
}

/**
 * Retrieve all members of the current dependency group the provided child Checkable depend on.
 *
 * Note, in order to ease duplicated dependencies exhaustion, the returned members are sorted by the parent Checkable.
 * This way, all identical dependencies are placed next to each other, and you can easily consume them via a simple loop.
 *
 * @param child The child Checkable to look for.
 *
 * @return - Returns all members of the current dependency group the provided child depend on.
 */
std::vector<Dependency::Ptr> DependencyGroup::GetMembers(const Checkable* child) const
{
	std::lock_guard lock(m_Mutex);
	std::vector<Dependency::Ptr> members;
	for (auto& [_, dependencies] : m_Members) {
		auto [begin, end] = dependencies.equal_range(child);
		std::transform(begin, end, std::back_inserter(members), [](const auto& member) {
			return member.second;
		});
	}

	std::sort(members.begin(), members.end(), [](const Dependency::Ptr& lhs, const Dependency::Ptr& rhs) {
		return lhs->GetParent() < rhs->GetParent();
	});
	return members;
}

/**
 * Retrieve the number of members in the current dependency group.
 *
 * This function mainly exists for optimization purposes, i.e. instead of getting a copy of the members and
 * counting them, we can directly query the number of members.
 *
 * @return - Returns the number of members in the current dependency group.
 */
size_t DependencyGroup::GetMemberCount() const
{
	std::lock_guard lock(m_Mutex);
	return std::accumulate(
		m_Members.begin(),
		m_Members.end(),
		static_cast<size_t>(0),
		[](int sum, const std::pair<CompositeKeyType, MemberValueType>& pair) {
			return sum + pair.second.size();
		}
	);
}

/**
 * Add a member to the current dependency group.
 *
 * @param member The dependency to add to the dependency group.
 */
void DependencyGroup::AddMember(const Dependency::Ptr& member)
{
	std::lock_guard lock(m_Mutex);
	auto compositeKey(MakeCompositeKeyFor(member));
	if (auto it(m_Members.find(compositeKey)); it != m_Members.end()) {
		it->second.emplace(member->GetChild().get(), member.get());
	} else {
		m_CompositeKey = ""; // Invalidate the composite key cache (if any).
		m_Members.emplace(compositeKey, MemberValueType{{member->GetChild().get(), member.get()}});
	}
}

/**
 * Remove a member from the current dependency group.
 *
 * @param member The dependency to remove from the dependency group.
 */
void DependencyGroup::RemoveMember(const Dependency::Ptr& member)
{
	std::lock_guard lock(m_Mutex);
	if (auto it(m_Members.find(MakeCompositeKeyFor(member))); it != m_Members.end()) {
		auto [begin, end] = it->second.equal_range(member->GetChild().get());
		for (auto memberIt(begin); memberIt != end; ++memberIt) {
			if (memberIt->second == member) {
				// This will also remove the child Checkable from the multimap container
				// entirely if this was the last member of it.
				it->second.erase(memberIt);
				// If the composite key has no more members left, we can remove it entirely as well.
				if (it->second.empty()) {
					m_Members.erase(it);
					m_CompositeKey = ""; // Invalidate the composite key cache (if any).
				}
				return;
			}
		}
	}
}

/**
 * Move the members of the provided dependency group to the provided destination dependency group.
 *
 * @param dest The dependency group to move the members to.
 */
void DependencyGroup::MoveMembersTo(const DependencyGroup::Ptr& dest)
{
	VERIFY(this != dest); // Prevent from doing something stupid, i.e. deadlocking ourselves.

	std::lock_guard lock(m_Mutex);
	for (auto& [_, members] : m_Members) {
		std::for_each(members.begin(), members.end(), [&dest](const auto& member) {
			dest->AddMember(member.second);
		});
	}
}

/**
 * Set the Icinga DB identifier for the current dependency group.
 *
 * The only usage of this function is the Icinga DB feature used to cache the unique hash of this dependency groups.
 *
 * @param identifier The Icinga DB identifier to set.
 */
void DependencyGroup::SetIcingaDBIdentifier(const String& identifier)
{
	std::lock_guard lock(m_Mutex);
	m_IcingaDBIdentifier = identifier;
}

/**
 * Retrieve the Icinga DB identifier for the current dependency group.
 *
 * When the identifier is not already set by Icinga DB via the SetIcingaDBIdentifier method,
 * this will just return an empty string.
 *
 * @return - Returns the Icinga DB identifier for the current dependency group.
 */
String DependencyGroup::GetIcingaDBIdentifier() const
{
	std::lock_guard lock(m_Mutex);
	return m_IcingaDBIdentifier;
}

/**
 * Retrieve the (non-unique) name of the current dependency group.
 *
 * For explicitly configured redundancy groups, the name of the dependency group is the same as the one located
 * in the configuration files. For non-redundant dependencies, on the other hand, the name is always empty.
 *
 * @return - Returns the name of the current dependency group.
 */
const String& DependencyGroup::GetName() const
{
	// We don't need to lock the mutex here, as the name is set once during
	// the object construction and never changed afterwards.
	return m_Name;
}

/**
 * Retrieve the unique composite key of the current dependency group.
 *
 * The composite key consists of some unique data of the group members, and should be used to generate
 * a unique deterministic hash for the dependency group. Additionally, for explicitly configured redundancy
 * groups, the non-unique dependency group name is also included on top of the composite keys.
 *
 * @return - Returns the composite key of the current dependency group.
 */
String DependencyGroup::GetCompositeKey()
{
	std::lock_guard lock(m_Mutex);
	if (m_CompositeKey.IsEmpty()) {
		Array::Ptr data(new Array{GetName()});
		for (auto& [compositeKey, _] : m_Members) {
			auto [parent, tp, stateFilter, ignoreSoftStates] = compositeKey;
			data->Add(std::move(parent));
			data->Add(std::move(tp));
			data->Add(stateFilter);
			data->Add(ignoreSoftStates);
		}

		m_CompositeKey = PackObject(data);
	}

	return m_CompositeKey;
}

/**
 * Peek the name of the first parent Checkable in the member list.
 *
 * Returns an empty string if it doesn't have any members.
 *
 * @return String - The name of the first parent Checkable in the member list.
 */
String DependencyGroup::PeekParentCheckableName() const
{
	std::lock_guard lock(m_Mutex);
	return m_Members.empty() ? "" : std::get<0>(m_Members.begin()->first);
}

/**
 * Retrieve the state of the current dependency group.
 *
 * The state of the dependency group is determined based on the state of the members of the group.
 * This method returns a DependencyGroup::State::Unknown immediately when the group has no members.
 * Otherwise, a dependency group is considered unreachable when none of the members is reachable.
 * A reachable dependency group is failed when the edges connected to it are not available.
 *
 * @return - Returns the state of the current dependency group.
 */
DependencyGroup::State DependencyGroup::GetState(DependencyType dt, int rstack) const
{
	MembersMap members;
	{
		// We don't want to hold the mutex lock for the entire evaluation, thus we just need to operate on a copy.
		std::lock_guard lock(m_Mutex);
		members = m_Members;
	}

	State state(State::Unknown);
	for (auto& [_, dependencies] : members) {
		for (auto& [checkable, dependency] : dependencies) {
			if (!dependency->GetParent()->IsReachable(dt, rstack)) {
				if (!IsRedundancyGroup()) {
					return State::Unreachable;
				}
				state = State::UnreachableFailed;
				break; // Cache the state and continue with the next batch of group members.
			}

			if (!dependency->IsAvailable(dt)) {
				if (!IsRedundancyGroup()) {
					Log(LogDebug, "Checkable")
						<< "Non-redundant dependency '" << dependency->GetName() << "' failed for checkable '"
						<< checkable->GetName() << "': Marking as unreachable.";

					return State::Failed;
				}
				state = State::Failed;
				break; // Cache the state and continue with the next batch of group members.
			}

			state = State::ReachableOK;
			if (IsRedundancyGroup()) {
				return state; // We have found one functional path, so that's enough!
			}
			break; // Move on to the next batch of group members (next composite key).
		}
	}

	return state;
}
