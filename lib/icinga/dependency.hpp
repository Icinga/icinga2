/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "base/shared-object.hpp"
#include "icinga/i2-icinga.hpp"
#include "icinga/dependency-ti.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <map>
#include <tuple>
#include <unordered_map>
#include <variant>

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
	DECLARE_OBJECT(Dependency);
	DECLARE_OBJECTNAME(Dependency);

	intrusive_ptr<Checkable> GetParent() const;
	intrusive_ptr<Checkable> GetChild() const;

	TimePeriod::Ptr GetPeriod() const;

	bool IsAvailable(DependencyType dt) const;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);
	static void AssertNoCycles();

	/* Note: Only use them for unit test mocks. Prefer OnConfigLoaded(). */
	void SetParent(intrusive_ptr<Checkable> parent);
	void SetChild(intrusive_ptr<Checkable> child);

	/**
	 * Functor to compare the parent Checkable of a dependency with an explicitly provided parent Checkable.
	 *
	 * This is used to group dependencies by their parent Checkable (c++ 14 heterogeneous lookups).
	 */
	struct ParentComparator {
		bool operator()(const Dependency::Ptr& dependency, const Checkable::Ptr& parent) const
		{
			return dependency->GetParent() < parent;
		}

		bool operator()(const Checkable::Ptr& parent, const Dependency::Ptr& rhs) const
		{
			return parent < rhs->GetParent();
		}
	};

protected:
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	Checkable::Ptr m_Parent;
	Checkable::Ptr m_Child;

	// Whether we've already performed the cyclic reference check for all dependencies on Icinga 2 startup.
	static bool m_CyclicReferenceForAllDependenciesAsserted;

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule, bool skipFilter = false);
};

/**
 * A DependencyGroup represents a set of dependencies that are somehow related to each other.
 *
 * The main purpose of this class is grouping all dependency objects that are related to each other in some way.
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
 * group wouldn't just contain 2 members, but 4 members in total, i.e. 2 for each child Checkable (C1 -> {P1, P2} and
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
	 * This tuple consists of the dependency parent Checkable name, the dependency time period
	 * name (empty if not configured), the state filter, and the ignore soft states flag.
	 *
	 * Note: The reason for not using the actual Checkable and TimePeriod object pointers here is to maintain
	 * the order of the composite keys, since they are used to uniquely identify the dependency group. However,
	 * the order of the keys is not guaranteed when using the actual objects, as they won't point to the same
	 * address after each reload.
	 */
	using CompositeKeyType = std::tuple<String, String, int, bool>;

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

	DependencyGroup(String name, const Dependency::Ptr& member);

	static void Register(const Dependency::Ptr& dependency);
	static void Unregister(const Dependency::Ptr& dependency);
	static size_t GetRegistrySize();

	static CompositeKeyType MakeCompositeKeyFor(const Dependency::Ptr& dependency);

	/**
	 * Check whether the current dependency group represents an explicitly configured redundancy group.
	 *
	 * @return bool - Returns true if it's a redundancy group, false otherwise.
	 */
	inline bool IsRedundancyGroup() const
	{
		return !m_Name.IsEmpty();
	}

	bool HasMembers() const;
	bool HasIdenticalMember(const Dependency::Ptr& dependency) const;
	std::vector<Dependency::Ptr> GetMembers(const Checkable* child) const;
	size_t GetMemberCount() const;

	void SetIcingaDBIdentifier(const String& identifier);
	String GetIcingaDBIdentifier() const;

	const String& GetName() const;
	String GetCompositeKey();

	enum State
	{
		Unknown           = 1ull << 0,
		Failed            = 1ull << 1,
		Unreachable       = 1ull << 2,
		ReachableOK       = 1ull << 3,
		UnreachableFailed = Unreachable | Failed,
	};

	State GetState(DependencyType dt = DependencyState, int rstack = 0) const;

	static boost::signals2::signal<void (const Dependency::Ptr&, const std::vector<DependencyGroup::Ptr>&)> OnMembersChanged;

protected:
	void AddMember(const Dependency::Ptr& member);
	void RemoveMember(const Dependency::Ptr& member);
	void MoveMembersTo(const DependencyGroup::Ptr& other);

	static void RefreshRegistry(const Dependency::Ptr& dependency, bool unregister);

private:
	mutable std::mutex m_Mutex;
	String m_IcingaDBIdentifier;
	String m_Name;
	String m_CompositeKey;
	MembersMap m_Members;

	using RegistryType = boost::multi_index_container<
		DependencyGroup*, // The type of the elements stored in the container.
		boost::multi_index::indexed_by<
			// This unique index allows to search/erase dependency groups by their composite key in an efficient manner.
			boost::multi_index::hashed_unique<
				boost::multi_index::mem_fun<DependencyGroup, String, &DependencyGroup::GetCompositeKey>,
				std::hash<String>
			>,
			// This non-unique index allows to search for dependency groups by their name, and reduces the overall
			// runtime complexity. Without this index, we would have to iterate over all elements to find the one
			// with the desired members and since containers don't allow erasing elements while iterating, we would
			// have to copy each of them to a temporary container, and then erase and reinsert them back to the original
			// container. This produces way too much overhead, and slows down the startup time of Icinga 2 significantly.
			boost::multi_index::hashed_non_unique<
				boost::multi_index::const_mem_fun<DependencyGroup, const String&, &DependencyGroup::GetName>,
				std::hash<String>
			>
		>
	>;

	// The global registry of dependency groups.
	static std::mutex m_RegistryMutex;
	static RegistryType m_Registry;
};

}

#endif /* DEPENDENCY_H */
