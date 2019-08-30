/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "icinga/dependency-ti.cpp"
#include "icinga/service.hpp"
#include "base/defer.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <set>
#include <sstream>

using namespace icinga;

REGISTER_TYPE(Dependency);

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

void Dependency::BlameInvalidParents(const std::vector<size_t>& currentBranch)
{
	std::ostringstream oss;

	oss << "Dependency#parents";

	for (auto idx : currentBranch) {
		oss << ".of[" << idx << ']';
	}

	oss << R"EOF( must be like one of "host_name", "host_name!service_name", { host = "host_name" }, { host = "host_name"; service = "service_name" }, { require = "any"; of = [ ... ] }.)EOF";

	BOOST_THROW_EXCEPTION(ScriptError(oss.str(), GetDebugInfo()));
}

// We don't have to allocate these strings every time, once is enough.
static const struct {
	String Require = "require";
	String Of = "of";
	String Host = "host";
	String Service = "service";

	std::set<String> RequireOptions {"all", "any"};
} l_ParentsStrings;

void Dependency::ValidateParentsRecursively(const Value& parents, std::vector<size_t>& currentBranch)
{
	if (parents.IsString()) {
		if (parents.Get<String>().IsEmpty()) {
			BlameInvalidParents(currentBranch);
		}
	} else if (parents.IsObject()) {
		auto dict (dynamic_pointer_cast<Dictionary>(parents.Get<Object::Ptr>()));

		if (!dict) {
			BlameInvalidParents(currentBranch);
		}

		Value require;
		if (dict->Get(l_ParentsStrings.Require, &require)) {
			Value of;
			if (!dict->Get(l_ParentsStrings.Of, &of) || dict->GetLength() > 2) {
				BlameInvalidParents(currentBranch);
			}

			if (!require.IsString() || !of.IsObject()) {
				BlameInvalidParents(currentBranch);
			}

			if (l_ParentsStrings.RequireOptions.find(require.Get<String>()) == l_ParentsStrings.RequireOptions.end()) {
				BlameInvalidParents(currentBranch);
			}

			auto array (dynamic_pointer_cast<Array>(of.Get<Object::Ptr>()));

			if (!array) {
				BlameInvalidParents(currentBranch);
			}

			ObjectLock arrayLock (array);
			size_t idx = 0;

			for (auto& val : array) {
				currentBranch.emplace_back(idx);
				Defer popBack ([&currentBranch](){ currentBranch.pop_back(); });

				ValidateParentsRecursively(val, currentBranch);

				++idx;
			}
		} else {
			Value host;
			if (dict->Get(l_ParentsStrings.Host, &host)) {
				Value service;
				if (dict->Get(l_ParentsStrings.Service, &service)) {
					if (dict->GetLength() > 2) {
						BlameInvalidParents(currentBranch);
					}

					if (!host.IsString() || host.Get<String>().IsEmpty()) {
						BlameInvalidParents(currentBranch);
					}
				} else {
					if (dict->GetLength() > 1) {
						BlameInvalidParents(currentBranch);
					}
				}

				if (!host.IsString() || host.Get<String>().IsEmpty()) {
					BlameInvalidParents(currentBranch);
				}
			} else {
				BlameInvalidParents(currentBranch);
			}
		}
	} else {
		BlameInvalidParents(currentBranch);
	}
}

void Dependency::ValidateParents(const Lazy<Value>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Dependency>::ValidateParents(lvalue, utils);

	auto& parents (lvalue());

	if (!parents.IsEmpty()) {
		std::vector<size_t> currentBranch;
		ValidateParentsRecursively(parents, currentBranch);
	}
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

void Dependency::BlameBadParents(String checkable)
{
	BOOST_THROW_EXCEPTION(ScriptError("Dependency '" + GetName() + "' references a parent host/service which doesn't exist: '" + std::move(checkable) + "'", GetDebugInfo()));
}

void Dependency::RequireParents(const Value& parents)
{
	if (parents.IsString()) {
		auto checkableName (parents.Get<String>());

		if (!(Service::GetByName(checkableName) || Host::GetByName(checkableName))) {
			BlameBadParents(std::move(checkableName));
		}
	} else {
		auto dict (static_pointer_cast<Dictionary>(parents.Get<Object::Ptr>()));

		Value hostSpec;
		if (dict->Get(l_ParentsStrings.Host, &hostSpec)) {
			auto hostName (hostSpec.Get<String>());
			auto host (Host::GetByName(hostName));

			if (!host) {
				BlameBadParents(std::move(hostName));
			}

			Value serviceSpec;
			if (dict->Get(l_ParentsStrings.Service, &serviceSpec)) {
				auto serviceName (serviceSpec.Get<String>());

				if (!host->GetServiceByShortName(serviceName)) {
					BlameBadParents(std::move(hostName) + "!" + std::move(serviceName));
				}
			}
		} else {
			auto of (static_pointer_cast<Array>(dict->Get(l_ParentsStrings.Of).Get<Object::Ptr>()));
			ObjectLock ofLock (of);

			for (auto& val : of) {
				RequireParents(val);
			}
		}
	}
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

	m_Child->AddDependency(this);

	Host::Ptr parentHost = Host::GetByName(GetParentHostName());

	if (parentHost) {
		if (GetParentServiceName().IsEmpty())
			m_Parent = parentHost;
		else
			m_Parent = parentHost->GetServiceByShortName(GetParentServiceName());
	}

	if (!m_Parent)
		BOOST_THROW_EXCEPTION(ScriptError("Dependency '" + GetName() + "' references a parent host/service which doesn't exist.", GetDebugInfo()));

	m_Parent->AddReverseDependency(this);

	auto parents (GetParents());

	if (!parents.IsEmpty()) {
		RequireParents(parents);
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

