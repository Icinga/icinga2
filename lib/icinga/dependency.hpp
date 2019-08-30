/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/dependency-ti.hpp"
#include <memory>
#include <set>
#include <utility>
#include <vector>

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
	bool IsAvailable(const Checkable::Ptr& parent, DependencyType dt) const;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

	void SetParents(const Value& value, bool suppress_events = false, const Value& cookie = Empty) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

protected:
	void ValidateParents(const Lazy<Value>& lvalue, const ValidationUtils& utils) override;
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Stop(bool runtimeRemoved) override;

private:
	class ParentsTree
	{
	public:
		ParentsTree(const ParentsTree&) = delete;
		ParentsTree(ParentsTree&&) = delete;
		ParentsTree& operator=(const ParentsTree&) = delete;
		ParentsTree& operator=(ParentsTree&&) = delete;
		virtual ~ParentsTree() = default;

		virtual void GetAllLeavesFlat(std::set<Checkable::Ptr>& out) const = 0;
		virtual bool IsAvailable(DependencyType dt) const = 0;

	protected:
		ParentsTree() = default;
	};

	class ParentsLeaf : public ParentsTree
	{
	public:
		inline ParentsLeaf(Dependency *dep, Checkable::Ptr checkable)
			: m_Dep(dep), m_Checkable(std::move(checkable))
		{
		}

		void GetAllLeavesFlat(std::set<Checkable::Ptr>& out) const override;
		bool IsAvailable(DependencyType dt) const override;

	private:
		Dependency *m_Dep;
		Checkable::Ptr m_Checkable;
	};

	class ParentsBranch : public ParentsTree
	{
	public:
		inline ParentsBranch(std::vector<std::unique_ptr<ParentsTree>> subTrees) : m_SubTrees(std::move(subTrees))
		{
		}

		void GetAllLeavesFlat(std::set<Checkable::Ptr>& out) const override;

	protected:
		std::vector<std::unique_ptr<ParentsTree>> m_SubTrees;
	};

	class ParentsAll : public ParentsBranch
	{
	public:
		using ParentsBranch::ParentsBranch;

		bool IsAvailable(DependencyType dt) const override;
	};

	class ParentsAny : public ParentsBranch
	{
	public:
		using ParentsBranch::ParentsBranch;

		bool IsAvailable(DependencyType dt) const override;
	};

	Checkable::Ptr m_Parent;
	Checkable::Ptr m_Child;
	std::unique_ptr<ParentsTree> m_ParentsTree;
	bool m_ParentsTreeValidated = false;

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule);

	void ValidateParentsRecursively(const Value& parents, std::vector<size_t>& currentBranch);
	void BlameInvalidParents(const std::vector<size_t>& currentBranch);
	std::unique_ptr<ParentsTree> RequireParents(const Value& parents);
	void BlameBadParents(String checkable);
	void SetParentsTree(std::unique_ptr<ParentsTree> parentsTree);
};

}

#endif /* DEPENDENCY_H */
