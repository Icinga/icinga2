/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "icinga/dependency-ti.cpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/initialize.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <sstream>
#include <unordered_map>
#include <utility>

using namespace icinga;

REGISTER_TYPE(Dependency);

class DependencyCycleChecker
{
	struct Node
	{
		bool m_Visited = false;
		bool m_OnStack = false;
		std::vector<Dependency::Ptr> m_ExtraDependencies;
	};

	std::unordered_map<Checkable::Ptr, Node> m_Nodes;
	std::vector<std::variant<Dependency::Ptr, Service::Ptr>> m_Stack;

public:
	void AddExtraDependency(Dependency::Ptr dependency)
	{
		auto& node = m_Nodes[dependency->GetChild()];
		node.m_ExtraDependencies.push_back(std::move(dependency));
	}

	void AssertNoCycle(const Checkable::Ptr& checkable)
	{
		auto& node = m_Nodes[checkable];

		Log(LogInformation, "DependencyCycleChecker") << m_Stack.size() << " " << checkable->GetName();

		if (node.m_OnStack) {
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

				s << "\n\t- "
					<< child->GetReflectionType()->GetName() << " " << std::quoted(std::string(child->GetName()))
					<< " depends on "
					<< parent->GetReflectionType()->GetName() << " " << std::quoted(std::string(parent->GetName()));
				if (dependency) {
					s << " (Dependency " << std::quoted(std::string(dependency->GetName())) << ")";
				} else {
					s << " (implicit)";
				}
			}
			BOOST_THROW_EXCEPTION(ScriptError(s.str()));
		}

		if (node.m_Visited) {
			return;
		}
		node.m_Visited = true;

		node.m_OnStack = true;

		// Implicit dependency of each service to its host
		if (auto service (dynamic_pointer_cast<Service>(checkable)); service) {
			m_Stack.emplace_back(service);
			AssertNoCycle(service->GetHost());
			m_Stack.pop_back();
		}

		// Explicitly configured dependency objects
		for (const auto& dep : checkable->GetDependencies(/* includePending = */ true)) {
			m_Stack.emplace_back(dep);
			AssertNoCycle(dep->GetParent());
			m_Stack.pop_back();
		}

		// Additional dependencies to consider
		for (const auto& dep : node.m_ExtraDependencies) {
			m_Stack.emplace_back(dep);
			AssertNoCycle(dep->GetParent());
			m_Stack.pop_back();
		}

		node.m_OnStack = false;
	}
};

INITIALIZE_ONCE([] {
	dynamic_cast<ConfigType*>(Dependency::TypeInstance.get())->OnBulkConfigLoaded.connect([](const std::vector<ConfigObject::Ptr>& objects) {
		DependencyCycleChecker checker;

		for (const ConfigObject::Ptr& object : objects) {
			Dependency::Ptr dependency = dynamic_pointer_cast<Dependency>(object);
			VERIFY(dependency != nullptr);
			dependency->InitChildParentReferences();
			checker.AddExtraDependency(dependency);
		}

		for (const ConfigObject::Ptr& object : objects) {
			checker.AssertNoCycle(dynamic_pointer_cast<Dependency>(object)->GetParent());
		}

		Log(LogInformation, "Dependency") << "OnBulkConfigLoaded(<" << objects.size() << " objects>)";
	});
});

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

	// Icinga DB will implicitly send config updates for the parent Checkable to refresh its affects_children and
	// affected_children columns when registering the dependency from the child Checkable. So, we need to register
	// the dependency from the parent Checkable first, otherwise the config update of the parent Checkable will change
	// nothing at all.
	m_Parent->AddReverseDependency(this);
	m_Child->AddDependency(this);
}

void Dependency::Stop(bool runtimeRemoved)
{
	ObjectImpl<Dependency>::Stop(runtimeRemoved);

	// Icinga DB will implicitly send config updates for the parent Checkable to refresh its affects_children and
	// affected_children columns when removing the dependency from the child Checkable. So, we need to remove the
	// dependency from the parent Checkable first, otherwise the config update of the parent Checkable will change
	// nothing at all.
	GetParent()->RemoveReverseDependency(this);
	GetChild()->RemoveDependency(this, runtimeRemoved);
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
