/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "base/object-packer.hpp"

using namespace icinga;

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
			dependencyGroup->CopyDependenciesTo(existingGroup);
		}
	};

	// Retrieve all the dependency groups with the same redundancy group name of the provided dependency object.
	// This allows us to shorten the lookup for the _one_ optimal group to (un)register the dependency from/to.
	auto [begin, end] = m_Registry.get<1>().equal_range(dependency->GetRedundancyGroup());
	for (auto it(begin); it != end; ++it) {
		DependencyGroup::Ptr existingGroup(*it);
		auto child(dependency->GetChild());
		if (auto dependencies(existingGroup->GetDependenciesForChild(child.get())); !dependencies.empty()) {
			m_Registry.erase(existingGroup->GetCompositeKey()); // Will be re-registered when needed down below.
			if (unregister) {
				existingGroup->RemoveDependency(dependency);
				// Remove the connection between the child Checkable and the dependency group if it has no members
				// left or the above removed member was the only member of the group that the child depended on.
				if (existingGroup->IsEmpty() || dependencies.size() == 1) {
					child->RemoveDependencyGroup(existingGroup);
				}
			}

			size_t totalDependencies(existingGroup->GetDependenciesCount());
			// If the existing dependency group has an identical member already, or the child Checkable of the
			// dependency object is the only member of it (totalDependencies == dependencies.size()), we can simply
			// add the dependency object to the existing group.
			if (!unregister && (existingGroup->HasParentWithConfig(dependency) || totalDependencies == dependencies.size())) {
				existingGroup->AddDependency(dependency);
			} else if (!unregister || (dependencies.size() > 1 && totalDependencies >= dependencies.size())) {
				// The child Checkable is going to have a new dependency group, so we must detach the existing one.
				child->RemoveDependencyGroup(existingGroup);

				Ptr replacementGroup(unregister ? nullptr : new DependencyGroup(existingGroup->GetRedundancyGroupName(), dependency));
				for (auto& existingDependency : dependencies) {
					if (existingDependency != dependency) {
						existingGroup->RemoveDependency(existingDependency);
						if (replacementGroup) {
							replacementGroup->AddDependency(existingDependency);
						} else {
							replacementGroup = new DependencyGroup(existingGroup->GetRedundancyGroupName(), existingDependency);
						}
					}
				}

				child->AddDependencyGroup(replacementGroup);
				registerRedundancyGroup(replacementGroup);
			}

			if (!existingGroup->IsEmpty()) {
				registerRedundancyGroup(existingGroup);
			}
			return;
		}
	}

	if (!unregister) {
		// We couldn't find any existing dependency group to register the dependency to, so we must
		// initiate a new one and attach it to the child Checkable and register to the global registry.
		DependencyGroup::Ptr newGroup(new DependencyGroup(dependency->GetRedundancyGroup()));
		newGroup->AddDependency(dependency);
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

DependencyGroup::DependencyGroup(String name): m_RedundancyGroupName(std::move(name))
{
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
	DependencyGroup::Ptr thisPtr(this); // Just in case the Checkable below was our last reference.
	for (auto& [_, children] : m_Members) {
		Checkable::Ptr previousChild;
		for (auto& [checkable, dependency] : children) {
			dest->AddDependency(dependency);
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
