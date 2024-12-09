/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "base/object-packer.hpp"
#include <numeric>

using namespace icinga;

boost::signals2::signal<void (const Dependency::Ptr&, const std::vector<DependencyGroup::Ptr>&)> DependencyGroup::OnMembersChanged;

std::mutex DependencyGroup::m_RegistryMutex;
DependencyGroup::RegistryType DependencyGroup::m_Registry;

/**
 * Refresh the global registry of dependency groups.
 *
 * Registers the provided dependency object to an existing dependency group with the same redundancy
 * group name (if any), or creates a new one and registers it to the child Checkable and the registry.
 *
 * Note: This is a helper function intended for internal use only, and you should acquire the global registry mutex
 * before calling this function.
 *
 * @param dependency The dependency object to refresh the registry for.
 * @param unregister A flag indicating whether the provided dependency object should be unregistered from the registry.
 */
void DependencyGroup::RefreshRegistry(const Dependency::Ptr& dependency, bool unregister)
{
	auto registerRedundancyGroup = [](const DependencyGroup::Ptr& dependencyGroup) {
		if (auto [it, inserted](m_Registry.insert(dependencyGroup.get())); !inserted) {
			DependencyGroup::Ptr existingGroup(*it);
			dependencyGroup->MoveMembersTo(existingGroup);
		}
	};

	// Retrieve all the dependency groups with the same redundancy group name of the provided dependency object.
	// This allows us to shorten the lookup for the _one_ optimal group to (un)register the dependency from/to.
	auto [begin, end] = m_Registry.get<1>().equal_range(dependency->GetRedundancyGroup());
	for (auto it(begin); it != end; ++it) {
		DependencyGroup::Ptr existingGroup(*it);
		auto child(dependency->GetChild());
		if (auto members(existingGroup->GetMembers(child.get())); !members.empty()) {
			m_Registry.erase(existingGroup->GetCompositeKey()); // Will be re-registered when needed down below.
			if (unregister) {
				existingGroup->RemoveMember(dependency);
				// Remove the connection between the child Checkable and the dependency group if it has no members
				// left or the above removed member was the only member of the group that the child depended on.
				if (!existingGroup->HasMembers() || members.size() == 1) {
					child->RemoveDependencyGroup(existingGroup);
				}
			}

			size_t totalMembers(existingGroup->GetMemberCount());
			// If the existing dependency group has an identical member already, or the child Checkable of the
			// dependency object is the only member of it (totalMembers == members.size()), we can simply add the
			// dependency object to the existing group.
			if (!unregister && (existingGroup->HasIdenticalMember(dependency) || totalMembers == members.size())) {
				existingGroup->AddMember(dependency);
			} else if (!unregister || (members.size() > 1 && totalMembers >= members.size())) {
				// The child Checkable is going to have a new dependency group, so we must detach the existing one.
				child->RemoveDependencyGroup(existingGroup);

				Ptr replacementGroup(unregister ? nullptr : new DependencyGroup(existingGroup->GetName(), dependency));
				for (auto& member : members) {
					if (member != dependency) {
						existingGroup->RemoveMember(member);
						if (replacementGroup) {
							replacementGroup->AddMember(member);
						} else {
							replacementGroup = new DependencyGroup(existingGroup->GetName(), member);
						}
					}
				}

				child->AddDependencyGroup(replacementGroup);
				registerRedundancyGroup(replacementGroup);
			}

			if (existingGroup->HasMembers()) {
				registerRedundancyGroup(existingGroup);
			}
			return;
		}
	}

	if (!unregister) {
		// We couldn't find any existing dependency group to register the dependency to, so we must
		// initiate a new one and attach it to the child Checkable and register to the global registry.
		DependencyGroup::Ptr newGroup(new DependencyGroup(dependency->GetRedundancyGroup(), dependency));
		dependency->GetChild()->AddDependencyGroup(newGroup);
		registerRedundancyGroup(newGroup);
	}
}

/**
 * Register the provided dependency to the global dependency group registry.
 *
 * @param dependency The dependency to register.
 */
void DependencyGroup::Register(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_RegistryMutex);
	RefreshRegistry(dependency, false);
}

/**
 * Unregister the provided dependency from the dependency group it was member of.
 *
 * @param dependency The dependency to unregister.
 */
void DependencyGroup::Unregister(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_RegistryMutex);
	RefreshRegistry(dependency, true);
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
		[](size_t sum, const std::pair<CompositeKeyType, MemberValueType>& pair) {
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
	DependencyGroup::Ptr thisPtr(this); // Just in case the Checkable below was our last reference.
	for (auto& [_, members] : m_Members) {
		Checkable::Ptr previousChild;
		for (auto& [checkable, dependency] : members) {
			dest->AddMember(dependency);
			if (!previousChild || previousChild != checkable) {
				previousChild = dependency->GetChild();
				previousChild->RemoveDependencyGroup(thisPtr);
				previousChild->AddDependencyGroup(dest);
			}
		}
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
