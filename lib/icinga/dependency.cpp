/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "icinga/dependency-ti.cpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/initialize.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <map>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <variant>

using namespace icinga;

REGISTER_TYPE(Dependency);

INITIALIZE_ONCE(&Dependency::StaticInitialize);

void Dependency::StaticInitialize()
{
	ConfigType::Get<Dependency>()->BeforeOnAllConfigLoaded.connect(&BeforeOnAllConfigLoadedHandler);
}

/**
 * Helper class to search for cycles in the dependency graph.
 *
 * State is stored inside the class and no synchronization is done,
 * hence instances of this class must not be used concurrently.
 */
class DependencyCycleChecker
{
	struct Node
	{
		bool Visited = false;
		bool OnStack = false;
		std::vector<Dependency::Ptr> ExtraDependencies;
	};

	std::unordered_map<Checkable::Ptr, Node> m_Nodes;

	// Stack representing the path currently visited by AssertNoCycle(). Dependency::Ptr represents an edge from its
	// child to parent, Service::Ptr represents the implicit dependency of that service to its host.
	std::vector<std::variant<Dependency::Ptr, Service::Ptr>> m_Stack;

public:
	/**
	 * Add a dependency to this DependencyCycleChecker that will be considered by AssertNoCycle() in addition to
	 * dependencies already registered to the checkables. This allows checking if additional dependencies would cause
	 * a cycle before actually registering them to the checkables.
	 *
	 * @param dependency Dependency to additionally consider during the cycle search.
	 */
	void AddExtraDependency(Dependency::Ptr dependency)
	{
		auto& node = m_Nodes[dependency->GetChild()];
		node.ExtraDependencies.emplace_back(std::move(dependency));
	}

	/**
	 * Searches the dependency graph for cycles and throws an exception if one is found.
	 *
	 * Only the part of the graph that's reachable from the starting node when traversing dependencies towards the
	 * parents is searched. In order to check that there are no cycles in the whole dependency graph, this method
	 * has to be called for every checkable. For this, the method can be called on the same DependencyCycleChecker
	 * instance multiple times, in which case parts of the graph aren't searched twice. However, if the graph structure
	 * changes, a new DependencyCycleChecker instance must be used.
	 *
	 * @param checkable Starting node for the cycle search.
	 * @throws ScriptError A dependency cycle was found.
	 */
	void AssertNoCycle(const Checkable::Ptr& checkable)
	{
		auto& node = m_Nodes[checkable];

		if (node.OnStack) {
			std::ostringstream s;
			s << "Dependency cycle:";
			for (const auto& obj : m_Stack) {
				Checkable::Ptr child, parent;
				Dependency::Ptr dependency;

				if (std::holds_alternative<Dependency::Ptr>(obj)) {
					dependency = std::get<Dependency::Ptr>(obj);
					parent = dependency->GetParent();
					child = dependency->GetChild();
				} else {
					const auto& service = std::get<Service::Ptr>(obj);
					parent = service->GetHost();
					child = service;
				}

				auto quoted = [](const String& str) { return std::quoted(str.GetData()); };
				s << "\n\t- " << child->GetReflectionType()->GetName() << " " << quoted(child->GetName()) << " depends on ";
				if (child == parent) {
					s << "itself";
				} else {
					s << parent->GetReflectionType()->GetName() << " " << quoted(parent->GetName());
				}
				if (dependency) {
					s << " (Dependency " << quoted(dependency->GetShortName()) << " " << dependency->GetDebugInfo() << ")";
				} else {
					s << " (implicit)";
				}
			}
			BOOST_THROW_EXCEPTION(ScriptError(s.str()));
		}

		if (node.Visited) {
			return;
		}
		node.Visited = true;

		node.OnStack = true;

		// Implicit dependency of each service to its host
		if (auto service (dynamic_pointer_cast<Service>(checkable)); service) {
			m_Stack.emplace_back(service);
			AssertNoCycle(service->GetHost());
			m_Stack.pop_back();
		}

		// Explicitly configured dependency objects
		for (const auto& dep : checkable->GetDependencies()) {
			m_Stack.emplace_back(dep);
			AssertNoCycle(dep->GetParent());
			m_Stack.pop_back();
		}

		// Additional dependencies to consider
		for (const auto& dep : node.ExtraDependencies) {
			m_Stack.emplace_back(dep);
			AssertNoCycle(dep->GetParent());
			m_Stack.pop_back();
		}

		node.OnStack = false;
	}
};

/**
 * Checks that adding these new dependencies to the configuration does not introduce any cycles.
 *
 * This is done as an optimization: cycles are checked once for all dependencies in a batch of config objects instead
 * of individually per dependency in Dependency::OnAllConfigLoaded(). For runtime updates, this function may still be
 * called for single objects.
 *
 * @param items Config items containing Dependency objects added to the running configuration.
 */
void Dependency::BeforeOnAllConfigLoadedHandler(const ConfigItems& items)
{
	DependencyCycleChecker checker;

	// Resolve parent/child names to Checkable::Ptr and temporarily add the edges to the checker.
	// The dependencies are later registered to the checkables by Dependency::OnAllConfigLoaded().
	items.ForEachObject<Dependency>([&checker](Dependency::Ptr dependency) {
		dependency->InitChildParentReferences();
		checker.AddExtraDependency(std::move(dependency));
	});

	// It's sufficient to search for cycles starting from newly added dependencies only: if a newly added dependency is
	// part of a cycle, that cycle is reachable from both the child and the parent of that dependency. The cycle search
	// is started from the parent as a slight optimization as that will traverse fewer edges if there is no cycle.
	items.ForEachObject<Dependency>([&checker](const Dependency::Ptr& dependency) {
		checker.AssertNoCycle(dependency->GetParent());
	});
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

void Dependency::InitChildParentReferences()
{
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
}

void Dependency::OnAllConfigLoaded()
{
	ObjectImpl<Dependency>::OnAllConfigLoaded();

	// InitChildParentReferences() has to be called before.
	VERIFY(m_Child && m_Parent);

	m_Child->AddDependency(this);
	m_Parent->AddReverseDependency(this);
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
