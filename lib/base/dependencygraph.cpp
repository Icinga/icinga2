/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dependencygraph.hpp"

using namespace icinga;

std::mutex DependencyGraph::m_Mutex;
DependencyGraph::DependencyMap DependencyGraph::m_Dependencies;

void DependencyGraph::AddDependency(ConfigObject* child, ConfigObject* parent)
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	if (auto [it, inserted] = m_Dependencies.insert(Edge(parent, child)); !inserted) {
		m_Dependencies.modify(it, [](Edge& e) { e.count++; });
	}
}

void DependencyGraph::RemoveDependency(ConfigObject* child, ConfigObject* parent)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	if (auto it(m_Dependencies.find(Edge(parent, child))); it != m_Dependencies.end()) {
		// A number <= 1 means, this isn't referenced by anyone and should be erased from the container.
		if (it->count == 1) {
			m_Dependencies.erase(it);
			return;
		}

		// Otherwise, each remove operation will should decrement this by 1 till it reaches <= 1
		// and causes the edge to completely be erased from the container.
		m_Dependencies.modify(it, [](Edge& e) { e.count--; });
	}
}

/**
 * Returns all the parent objects of the given child object.
 *
 * @param child The child object.
 *
 * @returns A list of the parent objects.
 */
std::vector<ConfigObject::Ptr> DependencyGraph::GetParents(const ConfigObject::Ptr& child)
{
	std::vector<ConfigObject::Ptr> objects;

	std::unique_lock lock(m_Mutex);
	auto [begin, end] = m_Dependencies.get<2>().equal_range(child.get());
	std::transform(begin, end, std::back_inserter(objects), [](const Edge& edge) {
		return edge.parent;
	});

	return objects;
}

/**
 * Returns all the dependent objects of the given parent object.
 *
 * @param parent The parent object.
 *
 * @returns A list of the dependent objects.
 */
std::vector<ConfigObject::Ptr> DependencyGraph::GetChildren(const ConfigObject::Ptr& parent)
{
	std::vector<ConfigObject::Ptr> objects;

	std::unique_lock lock(m_Mutex);
	auto [begin, end] = m_Dependencies.get<1>().equal_range(parent.get());
	std::transform(begin, end, std::back_inserter(objects), [](const Edge& edge) {
		return edge.child;
	});

	return objects;
}
