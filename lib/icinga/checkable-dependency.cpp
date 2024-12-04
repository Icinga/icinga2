/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/logger.hpp"
#include "base/object-packer.hpp"
#include "base/utility.hpp"
#include "icinga/dependency.hpp"
#include "icinga/service.hpp"
#include <unordered_map>
#include <numeric>

using namespace icinga;

// Is the default (dummy) redundancy group used to group non-redundant dependencies.
static String l_DefaultRedundancyGroup(Utility::NewUniqueID());

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
 * Retrieve the Icinga DB identifier of the provided redundancy group (if any).
 *
 * @param redundancyGroup The name of the redundancy group.
 *
 * @return RedundancyGroup - Returns the Icinga DB identifier of the given redundancy group.
 */
Shared<RedundancyGroup>::Ptr Checkable::GetRedundancyGroup(const String& redundancyGroup)
{
	std::lock_guard<std::mutex> lock(m_DependencyMutex);
	if (auto it(m_Dependencies.find(redundancyGroup)); it != m_Dependencies.end()) {
		return it->second;
	}

	return nullptr;
}

/**
 * Retrieve the Checkable dependencies grouped by redundancy group.
 *
 * Note, to simplify the implementation, non-redundant dependencies are grouped under a dummy group,
 * which is a randomly generated string. To verify if a given redundancy group is the default one, use
 * the RedundancyGroup::IsDefault() helper function.
 *
 * @return - Returns a map of redundancy groups and their member sets.
 */
std::unordered_map<String, Shared<RedundancyGroup>::Ptr> Checkable::GetRedundancyGroups() const
{
	std::lock_guard lock(m_DependencyMutex);
	return {m_Dependencies.begin(), m_Dependencies.end()};
}

void Checkable::AddDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	auto group(dep->GetRedundancyGroup().IsEmpty() ? l_DefaultRedundancyGroup : dep->GetRedundancyGroup());
	for (auto& redundancyGroup: m_Dependencies) {
		if (redundancyGroup->GetName() == group) {
			redundancyGroup->AddMember(dep);
			return;
		}
	}
	m_Dependencies.emplace(new RedundancyGroup(group, dep));
}

void Checkable::RemoveDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	auto groupName(dep->GetRedundancyGroup().IsEmpty() ? l_DefaultRedundancyGroup : dep->GetRedundancyGroup());
	for (auto& redundancyGroup: m_Dependencies) {
		if (redundancyGroup->GetName() == groupName) {
			redundancyGroup->RemoveMember(dep);
			if (!redundancyGroup->HasMembers()) {
				m_Dependencies.erase(redundancyGroup);
			}
			return;
		}
	}
}

std::vector<Dependency::Ptr> Checkable::GetDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	return std::accumulate(
		m_Dependencies.begin(),
		m_Dependencies.end(),
		std::vector<Dependency::Ptr>(),
		[](std::vector<Dependency::Ptr>& acc, const auto& pair) {
			auto members(pair.second->GetMembers());
			acc.insert(acc.end(), members.begin(), members.end());

			return acc;
		}
	);
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
