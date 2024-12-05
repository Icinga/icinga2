/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "base/object-packer.hpp"
#include <numeric>

using namespace icinga;

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
