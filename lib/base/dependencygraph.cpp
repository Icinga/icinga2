/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dependencygraph.hpp"

using namespace icinga;

std::mutex DependencyGraph::m_Mutex;
DependencyGraph::DependencyMap DependencyGraph::m_Dependencies;

void DependencyGraph::AddDependency(ConfigObject* child, ConfigObject* parent)
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	auto pair = m_Dependencies.insert(Edge(parent, child));
	if (!pair.second) {
		m_Dependencies.modify(pair.first, [](Edge& e) { e.count++; });
	}
}

void DependencyGraph::RemoveDependency(ConfigObject* child, ConfigObject* parent)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	auto it(m_Dependencies.find(Edge(parent, child)));
	if (it != m_Dependencies.end()) {
		if (it->count > 1) {
			// Remove a duplicate edge from child to node, i.e. decrement the corresponding counter.
			m_Dependencies.modify(it, [](Edge& e) { e.count--; });
		} else {
			// Remove the last edge from child to node (decrementing the counter would set it to 0),
			// thus remove that connection from the data structure completely.
			m_Dependencies.erase(it);
		}
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

	std::unique_lock<std::mutex> lock(m_Mutex);
	auto range = m_Dependencies.get<2>().equal_range(child.get());
	for (auto it(range.first); it != range.second; ++it) {
		objects.emplace_back(it->parent);
	}

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

	std::unique_lock<std::mutex> lock(m_Mutex);
	auto range = m_Dependencies.get<1>().equal_range(parent.get());
	for (auto it(range.first); it != range.second; ++it) {
		objects.emplace_back(it->child);
	}

	return objects;
}
