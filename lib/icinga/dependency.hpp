// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "base/shared-object.hpp"
#include "config/configitem.hpp"
#include "icinga/i2-icinga.hpp"
#include "icinga/dependency-ti.hpp"
#include "icinga/timeperiod.hpp"
#include <map>
#include <tuple>
#include <unordered_map>

namespace icinga
{

class ApplyRule;
struct ScriptFrame;
class Host;
class Service;

/**
 * A service dependency..
 *
 * @ingroup icinga
 */
class Dependency final : public ObjectImpl<Dependency>
{
public:
	static void StaticInitialize();

	DECLARE_OBJECT(Dependency);
	DECLARE_OBJECTNAME(Dependency);

	intrusive_ptr<Checkable> GetParent() const;
	intrusive_ptr<Checkable> GetChild() const;

	TimePeriod::Ptr GetPeriod() const;

	bool IsAvailable(DependencyType dt) const;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

	/* Note: Only use them for unit test mocks. Prefer InitChildParentReferences(). */
	void SetParent(intrusive_ptr<Checkable> parent);
	void SetChild(intrusive_ptr<Checkable> child);

	/**
	 * The maximum number of allowed dependency recursion levels.
	 *
	 * This is a subjective limit how deep the dependency tree should be allowed to go, as anything beyond this level
	 * is just madness and will likely result in a stack overflow or other undefined behavior.
	 */
	static constexpr int MaxDependencyRecursionLevel{256};

protected:
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Stop(bool runtimeRemoved) override;
	void InitChildParentReferences();

private:
	Checkable::Ptr m_Parent;
	Checkable::Ptr m_Child;

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule, bool skipFilter = false);

	static void BeforeOnAllConfigLoadedHandler(const ConfigItems& items);
};

/**
 * A DependencyGroup represents a set of dependencies that are somehow related to each other.
 *
 * Specifically, a DependencyGroup is a container for Dependency objects of different Checkables that share the same
 * child -> parent relationship config, thus forming a group of dependencies. All dependencies of a Checkable that
 * have the same "redundancy_group" attribute value set are guaranteed to be part of the same DependencyGroup object,
 * and another Checkable will join that group if and only if it has identical set of dependencies, that is, the same
 * parent(s), same redundancy group name and all other dependency attributes required to form a composite key.
 *
 * More specifically, let's say we have a dependency graph like this:
 * @verbatim
 *                      PP1              PP2
 *                      /\               /\
 *                      ||               ||
 *                  ––––||–––––––––––––––||–––––
 *                      P1 - ( "RG1" ) - P2
 *                  ––––––––––––––––––––––––––––
 *                          /\      /\
 *                          ||      ||
 *                          C1      C2
 * @endverbatim
 * The arrows represent a dependency relationship from bottom to top, i.e. both "C1" and "C2" depend on
 * their "RG1" redundancy group, and "P1" and "P2" depend each on their respective parents (PP1, PP2 - no group).
 * Now, as one can see, both "C1" and "C2" have identical dependencies, that is, they both depend on the same
 * redundancy group "RG1" (these could e.g. be constructed through some Apply Rules).
 *
 * So, instead of having to maintain two separate copies of that graph, we can bring that imaginary redundancy group
 * into reality by putting both "P1" and "P2" into an actual DependencyGroup object. However, we don't really put "P1"
 * and "P2" objects into that group, but rather the actual Dependency objects of both child Checkables. Therefore, the
 * group wouldn't just contain 2 dependencies, but 4 in total, i.e. 2 for each child Checkable (C1 -> {P1, P2} and
 * C2 -> {P1, P2}). This way, both child Checkables can just refer to that very same DependencyGroup object.
 *
 * However, since not all dependencies are part of a redundancy group, we also have to consider the case where
 * a Checkable has dependencies that are not part of any redundancy group, like P1 -> PP1. In such situations,
 * each of the child Checkables (e.g. P1, P2) will have their own (sharable) DependencyGroup object just like for RGs.
 * This allows us to keep the implementation simple and treat redundant and non-redundant dependencies in the same
 * way, without having to introduce any special cases everywhere. So, in the end, we'd have 3 dependency groups in
 * total, i.e. one for the redundancy group "RG1" (shared by C1 and C2), and two distinct groups for P1 and P2.
 *
 * @ingroup icinga
 */
class DependencyGroup final : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(DependencyGroup);

	/**
	 * Defines the key type of each dependency group members.
	 *
	 * This tuple consists of the dependency parent Checkable, the dependency time period (nullptr if not configured),
	 * the state filter, and the ignore soft states flag. Each of these values influences the availability of the
	 * dependency object, and thus used to group similar dependencies from different Checkables together.
	 */
	using CompositeKeyType = std::tuple<Checkable*, TimePeriod*, int, bool>;

	/**
	 * Represents the value type of each dependency group members.
	 *
	 * It stores the dependency objects of any given Checkable that produce the same composite key (CompositeKeyType).
	 * In other words, when looking at the dependency graph from the class description, the two dependency objects
	 * {C1, C2} -> P1 produce the same composite key, thus they are mapped to the same MemberValueType container with
	 * "C1" and "C2" as their keys respectively. Since Icinga 2 allows to construct different identical dependencies
	 * (duplicates), we're using a multimap instead of a simple map here.
	 */
	using MemberValueType = std::unordered_multimap<const Checkable*, Dependency*>;
	using MembersMap = std::map<CompositeKeyType, MemberValueType>;

	DependencyGroup(String name, const std::set<Dependency::Ptr>& dependencies);

	static DependencyGroup::Ptr Register(const DependencyGroup::Ptr& dependencyGroup);
	static std::pair<std::set<Dependency::Ptr>, bool> Unregister(const DependencyGroup::Ptr& dependencyGroup, const Checkable::Ptr& child);
	static size_t GetRegistrySize();

	static CompositeKeyType MakeCompositeKeyFor(const Dependency::Ptr& dependency);

	/**
	 * Check whether the current dependency group represents an explicitly configured redundancy group.
	 *
	 * @return bool - Returns true if it's a redundancy group, false otherwise.
	 */
	inline bool IsRedundancyGroup() const
	{
		return !m_RedundancyGroupName.IsEmpty();
	}

	bool HasChildren() const;
	void AddDependency(const Dependency::Ptr& dependency);
	void RemoveDependency(const Dependency::Ptr& dependency);
	std::vector<Dependency::Ptr> GetDependenciesForChild(const Checkable* child) const;
	void LoadParents(std::set<Checkable::Ptr>& parents) const;
	size_t GetDependenciesCount() const;

	void SetIcingaDBIdentifier(const String& identifier);
	String GetIcingaDBIdentifier() const;

	const String& GetRedundancyGroupName() const;
	String GetCompositeKey();

	enum class State { Ok, Failed, Unreachable };
	State GetState(const Checkable* child, DependencyType dt = DependencyState) const;

	static boost::signals2::signal<void(const Checkable::Ptr&, const DependencyGroup::Ptr&)> OnChildRegistered;
	static boost::signals2::signal<void(const DependencyGroup::Ptr&, const std::vector<Dependency::Ptr>&, bool)> OnChildRemoved;

private:
	void CopyDependenciesTo(const DependencyGroup::Ptr& dest);

	struct Hash
	{
		size_t operator()(const DependencyGroup::Ptr& dependencyGroup) const
		{
			size_t hash = std::hash<String>{}(dependencyGroup->GetRedundancyGroupName());
			for (const auto& [key, group] : dependencyGroup->m_Members) {
				boost::hash_combine(hash, key);
			}
			return hash;
		}
	};

	struct Equal
	{
		bool operator()(const DependencyGroup::Ptr& lhs, const DependencyGroup::Ptr& rhs) const
		{
			if (lhs->GetRedundancyGroupName() != rhs->GetRedundancyGroupName()) {
				return false;
			}

			return std::equal(
				lhs->m_Members.begin(), lhs->m_Members.end(),
				rhs->m_Members.begin(), rhs->m_Members.end(),
				[](const auto& l, const auto& r) { return l.first == r.first; }
			);
		}
	};

private:
	mutable std::mutex m_Mutex;
	/**
	 * This identifier is used by Icinga DB to cache the unique hash of this dependency group.
	 *
	 * For redundancy groups, once Icinga DB sets this identifier, it will never change again for the lifetime
	 * of the object. For non-redundant dependency groups, this identifier is (mis)used to cache the shared edge
	 * state ID of the group. Specifically, non-redundant dependency groups are irrelevant for Icinga DB, so since
	 * this field isn't going to be used for anything else, we use it to cache the computed shared edge state ID.
	 * Likewise, if that gets set, it will never change again for the lifetime of the object as well.
	 */
	String m_IcingaDBIdentifier;
	String m_RedundancyGroupName;
	MembersMap m_Members;

	using RegistryType = std::unordered_set<DependencyGroup::Ptr, Hash, Equal>;

	// The global registry of dependency groups.
	static std::mutex m_RegistryMutex;
	static RegistryType m_Registry;
};

/**
 * Helper class to evaluate the reachability of checkables and state of dependency groups.
 *
 * This class is used for implementing Checkable::IsReachable() and DependencyGroup::GetState().
 * For this, both methods call each other, traversing the dependency graph recursively. In order
 * to achieve linear runtime in the graph size, the class internally caches state information
 * (otherwise, evaluating the state of the same checkable multiple times can result in exponential
 * worst-case complexity). Because of this cached information is not invalidated, the object is
 * intended to be short-lived.
 */
class DependencyStateChecker
{
public:
	explicit DependencyStateChecker(DependencyType dt);

	bool IsReachable(Checkable::ConstPtr checkable, int rstack = 0);
	DependencyGroup::State GetState(const DependencyGroup::ConstPtr& group, const Checkable* child, int rstack = 0);

private:
	DependencyType m_DependencyType;
	std::unordered_map<Checkable::ConstPtr, bool> m_Cache;
};

}

#endif /* DEPENDENCY_H */
