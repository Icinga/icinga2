/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "base/object-packer.hpp"

using namespace icinga;

boost::signals2::signal<void(const Checkable::Ptr&, const DependencyGroup::Ptr&)> DependencyGroup::OnChildRegistered;
boost::signals2::signal<void(const DependencyGroup::Ptr&, const std::vector<Dependency::Ptr>&, bool)> DependencyGroup::OnChildRemoved;

std::mutex DependencyGroup::m_RegistryMutex;
DependencyGroup::RegistryType DependencyGroup::m_Registry;

/**
 * Register the provided dependency group to the global dependency group registry.
 *
 * In case there is already an identical dependency group in the registry, the provided dependency group is merged
 * with the existing one, and that group is returned. Otherwise, the provided dependency group is registered as is,
 * and it's returned.
 *
 * @param dependencyGroup The dependency group to register.
 */
DependencyGroup::Ptr DependencyGroup::Register(const DependencyGroup::Ptr& dependencyGroup)
{
	std::lock_guard lock(m_RegistryMutex);
	if (auto [it, inserted] = m_Registry.insert(dependencyGroup); !inserted) {
		dependencyGroup->CopyDependenciesTo(*it);
		return *it;
	}
	return dependencyGroup;
}

/**
 * Detach the provided child Checkable from the specified dependency group.
 *
 * Unregisters all the dependency objects the child Checkable depends on from the provided dependency group and
 * removes the dependency group from the global registry if it becomes empty afterward.
 *
 * @param dependencyGroup The dependency group to unregister the child Checkable from.
 * @param child The child Checkable to detach from the dependency group.
 *
 * @return - Returns the dependency objects of the child Checkable that were member of the provided dependency group
 *           and a boolean indicating whether the dependency group has been erased from the global registry.
 */
std::pair<std::set<Dependency::Ptr>, bool> DependencyGroup::Unregister(const DependencyGroup::Ptr& dependencyGroup, const Checkable::Ptr& child)
{
	std::lock_guard lock(m_RegistryMutex);
	if (auto it(m_Registry.find(dependencyGroup)); it != m_Registry.end()) {
		auto existingGroup(*it);
		auto dependencies(existingGroup->GetDependenciesForChild(child.get()));

		for (const auto& dependency : dependencies) {
			existingGroup->RemoveDependency(dependency);
		}

		if (existingGroup->IsEmpty()) {
			m_Registry.erase(it);
		}
		return {{dependencies.begin(), dependencies.end()}, existingGroup->IsEmpty()};
	}
	return {{}, false};
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

DependencyGroup::DependencyGroup(String name): m_RedundancyGroupName(std::move(name))
{
}

DependencyGroup::DependencyGroup(String name, const std::set<Dependency::Ptr>& dependencies): m_RedundancyGroupName(std::move(name))
{
	for (const auto& dependency : dependencies) {
		AddDependency(dependency);
	}
}

/**
 * Create a composite key for the provided dependency.
 *
 * The composite key consists of all the properties of the provided dependency object that influence its availability.
 *
 * @param dependency The dependency object to create a composite key for.
 *
 * @return - Returns the composite key for the provided dependency.
 */
DependencyGroup::CompositeKeyType DependencyGroup::MakeCompositeKeyFor(const Dependency::Ptr& dependency)
{
	return std::make_tuple(
		dependency->GetParent().get(),
		dependency->GetPeriod().get(),
		dependency->GetStateFilter(),
		dependency->GetIgnoreSoftStates()
	);
}

/**
 * Check if the current dependency group is empty.
 *
 * @return bool - Returns true if the current dependency group has no members, otherwise false.
 */
bool DependencyGroup::IsEmpty() const
{
	std::lock_guard lock(m_Mutex);
	return m_Members.empty();
}

/**
 * Retrieve all dependency objects of the current dependency group the provided child Checkable depend on.
 *
 * @param child The child Checkable to get the dependencies for.
 *
 * @return - Returns all the dependencies of the provided child Checkable in the current dependency group.
 */
std::vector<Dependency::Ptr> DependencyGroup::GetDependenciesForChild(const Checkable* child) const
{
	std::lock_guard lock(m_Mutex);
	std::vector<Dependency::Ptr> dependencies;
	for (auto& [_, children] : m_Members) {
		auto [begin, end] = children.equal_range(child);
		std::transform(begin, end, std::back_inserter(dependencies), [](const auto& pair) {
			return pair.second;
		});
	}
	return dependencies;
}

/**
 * Retrieve the number of dependency objects in the current dependency group.
 *
 * This function mainly exists for optimization purposes, i.e. instead of getting a copy of the members and
 * counting them, we can directly query the number of dependencies in the group.
 *
 * @return size_t
 */
size_t DependencyGroup::GetDependenciesCount() const
{
	std::lock_guard lock(m_Mutex);
	size_t count(0);
	for (auto& [_, dependencies] : m_Members) {
		count += dependencies.size();
	}
	return count;
}

/**
 * Add a dependency object to the current dependency group.
 *
 * @param dependency The dependency to add to the dependency group.
 */
void DependencyGroup::AddDependency(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_Mutex);
	auto compositeKey(MakeCompositeKeyFor(dependency));
	if (auto it(m_Members.find(compositeKey)); it != m_Members.end()) {
		it->second.emplace(dependency->GetChild().get(), dependency.get());
	} else {
		m_Members.emplace(compositeKey, MemberValueType{{dependency->GetChild().get(), dependency.get()}});
	}
}

/**
 * Remove a dependency object from the current dependency group.
 *
 * @param dependency The dependency to remove from the dependency group.
 */
void DependencyGroup::RemoveDependency(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_Mutex);
	if (auto it(m_Members.find(MakeCompositeKeyFor(dependency))); it != m_Members.end()) {
		auto [begin, end] = it->second.equal_range(dependency->GetChild().get());
		for (auto childrenIt(begin); childrenIt != end; ++childrenIt) {
			if (childrenIt->second == dependency) {
				// This will also remove the child Checkable from the multimap container
				// entirely if this was the last child of it.
				it->second.erase(childrenIt);
				// If the composite key has no more children left, we can remove it entirely as well.
				if (it->second.empty()) {
					m_Members.erase(it);
				}
				return;
			}
		}
	}
}

/**
 * Copy the dependency objects of the current dependency group to the provided dependency group (destination).
 *
 * @param dest The dependency group to move the dependencies to.
 */
void DependencyGroup::CopyDependenciesTo(const DependencyGroup::Ptr& dest)
{
	VERIFY(this != dest); // Prevent from doing something stupid, i.e. deadlocking ourselves.

	std::lock_guard lock(m_Mutex);
	for (auto& [_, children] : m_Members) {
		std::for_each(children.begin(), children.end(), [&dest](const auto& pair) {
			dest->AddDependency(pair.second);
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
 * Retrieve the redundancy group name of the current dependency group.
 *
 * If the current dependency group doesn't represent a redundancy group, this will return an empty string.
 *
 * @return - Returns the name of the current dependency group.
 */
const String& DependencyGroup::GetRedundancyGroupName() const
{
	// We don't need to lock the mutex here, as the name is set once during
	// the object construction and never changed afterwards.
	return m_RedundancyGroupName;
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
	// This a copy of the CompositeKeyType definition but with the String type instead of Checkable* and TimePeriod*.
	// This is because we need to produce a deterministic value from the composite key after each restart and that's
	// not achievable using pointers.
	using StringTuple = std::tuple<String, String, int, bool>;
	std::vector<StringTuple> compositeKeys;
	{
		std::lock_guard lock(m_Mutex);
		for (auto& [compositeKey, _] : m_Members) {
			auto [parent, tp, stateFilter, ignoreSoftStates] = compositeKey;
			compositeKeys.emplace_back(parent->GetName(), tp ? tp->GetName() : "", stateFilter, ignoreSoftStates);
		}
	}

	// IMPORTANT: The order of the composite keys must be sorted to ensure the deterministic hash value.
	std::sort(compositeKeys.begin(), compositeKeys.end());

	Array::Ptr data(new Array{GetRedundancyGroupName()});
	for (auto& compositeKey : compositeKeys) {
		auto [parent, tp, stateFilter, ignoreSoftStates] = compositeKey;
		data->Add(std::move(parent));
		data->Add(std::move(tp));
		data->Add(stateFilter);
		data->Add(ignoreSoftStates);
	}

	return PackObject(data);
}

/**
 * Retrieve the state of the current dependency group.
 *
 * The state of the dependency group is determined based on the state of the parent Checkables and dependency objects
 * of the group. A dependency group is considered unreachable when none of the parent Checkables is reachable. However,
 * a dependency group may still be marked as failed even when it has reachable parent Checkables, but an unreachable
 * group has always a failed state.
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

	State state{false /* Reachable */, false /* OK */};
	for (auto& [_, children] : members) {
		for (auto& [checkable, dependency] : children) {
			state.Reachable = dependency->GetParent()->IsReachable(dt, rstack);
			if (!state.Reachable && !IsRedundancyGroup()) {
				return state;
			}

			if (state.Reachable) {
				state.OK = dependency->IsAvailable(dt);
				// If this is a redundancy group, and we have found one functional path, that's enough and we can return.
				// Likewise, if this is a non-redundant dependency group, and we have found one non-functional path,
				// we have to mark the group as failed and return.
				if (state.OK == IsRedundancyGroup()) { // OK && IsRedundancyGroup() || !OK && !IsRedundancyGroup()
					return state;
				}
			}
			break; // Move on to the next batch of group members (next composite key).
		}
	}

	return state;
}
