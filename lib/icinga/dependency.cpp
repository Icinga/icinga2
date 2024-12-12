/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "icinga/dependency-ti.cpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/initialize.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/object-packer.hpp"
#include <map>
#include <sstream>
#include <utility>
#include <numeric>

using namespace icinga;

REGISTER_TYPE(Dependency);

bool Dependency::m_AssertNoCyclesForIndividualDeps = false;

struct DependencyCycleNode
{
	bool Visited = false;
	bool OnStack = false;
};

struct DependencyStackFrame
{
	ConfigObject::Ptr Node;
	bool Implicit;

	inline DependencyStackFrame(ConfigObject::Ptr node, bool implicit = false) : Node(std::move(node)), Implicit(implicit)
	{ }
};

struct DependencyCycleGraph
{
	std::map<Checkable::Ptr, DependencyCycleNode> Nodes;
	std::vector<DependencyStackFrame> Stack;
};

static void AssertNoDependencyCycle(const Checkable::Ptr& checkable, DependencyCycleGraph& graph, bool implicit = false);

static void AssertNoParentDependencyCycle(const Checkable::Ptr& parent, DependencyCycleGraph& graph, bool implicit)
{
	if (graph.Nodes[parent].OnStack) {
		std::ostringstream oss;
		oss << "Dependency cycle:\n";

		for (auto& frame : graph.Stack) {
			oss << frame.Node->GetReflectionType()->GetName() << " '" << frame.Node->GetName() << "'";

			if (frame.Implicit) {
				oss << " (implicit)";
			}

			oss << "\n-> ";
		}

		oss << parent->GetReflectionType()->GetName() << " '" << parent->GetName() << "'";

		if (implicit) {
			oss << " (implicit)";
		}

		BOOST_THROW_EXCEPTION(ScriptError(oss.str()));
	}

	AssertNoDependencyCycle(parent, graph, implicit);
}

static void AssertNoDependencyCycle(const Checkable::Ptr& checkable, DependencyCycleGraph& graph, bool implicit)
{
	auto& node (graph.Nodes[checkable]);

	if (!node.Visited) {
		node.Visited = true;
		node.OnStack = true;
		graph.Stack.emplace_back(checkable, implicit);

		for (auto& dep : checkable->GetDependencies()) {
			graph.Stack.emplace_back(dep);
			AssertNoParentDependencyCycle(dep->GetParent(), graph, false);
			graph.Stack.pop_back();
		}

		{
			auto service (dynamic_pointer_cast<Service>(checkable));

			if (service) {
				AssertNoParentDependencyCycle(service->GetHost(), graph, true);
			}
		}

		graph.Stack.pop_back();
		node.OnStack = false;
	}
}

void Dependency::AssertNoCycles()
{
	DependencyCycleGraph graph;

	for (auto& host : ConfigType::GetObjectsByType<Host>()) {
		AssertNoDependencyCycle(host, graph);
	}

	for (auto& service : ConfigType::GetObjectsByType<Service>()) {
		AssertNoDependencyCycle(service, graph);
	}

	m_AssertNoCyclesForIndividualDeps = true;
}

String DependencyNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Dependency::Ptr dependency = dynamic_pointer_cast<Dependency>(context);

	if (!dependency)
		return "";

	String name = dependency->GetChildHostName();

	if (!dependency->GetChildServiceName().IsEmpty())
		name += "!" + dependency->GetChildServiceName();

	name += "!" + shortName;

	return name;
}

Dictionary::Ptr DependencyNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens = name.Split("!");

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid Dependency name."));

	Dictionary::Ptr result = new Dictionary();
	result->Set("child_host_name", tokens[0]);

	if (tokens.size() > 2) {
		result->Set("child_service_name", tokens[1]);
		result->Set("name", tokens[2]);
	} else {
		result->Set("name", tokens[1]);
	}

	return result;
}

void Dependency::OnConfigLoaded()
{
	Value defaultFilter;

	if (GetParentServiceName().IsEmpty())
		defaultFilter = StateFilterUp;
	else
		defaultFilter = StateFilterOK | StateFilterWarning;

	SetStateFilter(FilterArrayToInt(GetStates(), Notification::GetStateFilterMap(), defaultFilter));
}

void Dependency::OnAllConfigLoaded()
{
	ObjectImpl<Dependency>::OnAllConfigLoaded();

	Host::Ptr childHost = Host::GetByName(GetChildHostName());

	if (childHost) {
		if (GetChildServiceName().IsEmpty())
			m_Child = childHost;
		else
			m_Child = childHost->GetServiceByShortName(GetChildServiceName());
	}

	if (!m_Child)
		BOOST_THROW_EXCEPTION(ScriptError("Dependency '" + GetName() + "' references a child host/service which doesn't exist.", GetDebugInfo()));

	Host::Ptr parentHost = Host::GetByName(GetParentHostName());

	if (parentHost) {
		if (GetParentServiceName().IsEmpty())
			m_Parent = parentHost;
		else
			m_Parent = parentHost->GetServiceByShortName(GetParentServiceName());
	}

	if (!m_Parent)
		BOOST_THROW_EXCEPTION(ScriptError("Dependency '" + GetName() + "' references a parent host/service which doesn't exist.", GetDebugInfo()));

	m_Child->AddDependency(this);
	m_Parent->AddReverseDependency(this);

	if (m_AssertNoCyclesForIndividualDeps) {
		DependencyCycleGraph graph;

		try {
			AssertNoDependencyCycle(m_Parent, graph);
		} catch (...) {
			m_Child->RemoveDependency(this);
			m_Parent->RemoveReverseDependency(this);
			throw;
		}
	}
}

void Dependency::Stop(bool runtimeRemoved)
{
	ObjectImpl<Dependency>::Stop(runtimeRemoved);

	GetChild()->RemoveDependency(this);
	GetParent()->RemoveReverseDependency(this);
}

bool Dependency::IsAvailable(DependencyType dt) const
{
	Checkable::Ptr parent = GetParent();

	Host::Ptr parentHost;
	Service::Ptr parentService;
	tie(parentHost, parentService) = GetHostService(parent);

	/* ignore if it's the same checkable object */
	if (parent == GetChild()) {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' passed: Parent and child " << (parentService ? "service" : "host") << " are identical.";
		return true;
	}

	/* ignore pending  */
	if (!parent->GetLastCheckResult()) {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' passed: Parent " << (parentService ? "service" : "host") << " '" << parent->GetName() << "' hasn't been checked yet.";
		return true;
	}

	if (GetIgnoreSoftStates()) {
		/* ignore soft states */
		if (parent->GetStateType() == StateTypeSoft) {
			Log(LogNotice, "Dependency")
				<< "Dependency '" << GetName() << "' passed: Parent " << (parentService ? "service" : "host") << " '" << parent->GetName() << "' is in a soft state.";
			return true;
		}
	} else {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' failed: Parent " << (parentService ? "service" : "host") << " '" << parent->GetName() << "' is in a soft state.";
	}

	int state;

	if (parentService)
		state = ServiceStateToFilter(parentService->GetState());
	else
		state = HostStateToFilter(parentHost->GetState());

	/* check state */
	if (state & GetStateFilter()) {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' passed: Parent " << (parentService ? "service" : "host") << " '" << parent->GetName() << "' matches state filter.";
		return true;
	}

	/* ignore if not in time period */
	TimePeriod::Ptr tp = GetPeriod();
	if (tp && !tp->IsInside(Utility::GetTime())) {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' passed: Outside time period.";
		return true;
	}

	if (dt == DependencyCheckExecution && !GetDisableChecks()) {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' passed: Checks are not disabled.";
		return true;
	} else if (dt == DependencyNotification && !GetDisableNotifications()) {
		Log(LogNotice, "Dependency")
			<< "Dependency '" << GetName() << "' passed: Notifications are not disabled";
		return true;
	}

	Log(LogNotice, "Dependency")
		<< "Dependency '" << GetName() << "' failed. Parent "
		<< (parentService ? "service" : "host") << " '" << parent->GetName() << "' is "
		<< (parentService ? Service::StateToString(parentService->GetState()) : Host::StateToString(parentHost->GetState()));

	return false;
}

Checkable::Ptr Dependency::GetChild() const
{
	return m_Child;
}

Checkable::Ptr Dependency::GetParent() const
{
	return m_Parent;
}

TimePeriod::Ptr Dependency::GetPeriod() const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

void Dependency::ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Dependency>::ValidateStates(lvalue, utils);

	int sfilter = FilterArrayToInt(lvalue(), Notification::GetStateFilterMap(), 0);

	if (GetParentServiceName().IsEmpty() && (sfilter & ~(StateFilterUp | StateFilterDown)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "states" }, "State filter is invalid for host dependency."));

	if (!GetParentServiceName().IsEmpty() && (sfilter & ~(StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "states" }, "State filter is invalid for service dependency."));
}

void Dependency::SetParent(intrusive_ptr<Checkable> parent)
{
	m_Parent = parent;
}

void Dependency::SetChild(intrusive_ptr<Checkable> child)
{
	m_Child = child;
}

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
