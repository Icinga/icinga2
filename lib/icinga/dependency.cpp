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
#include <utility>

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
