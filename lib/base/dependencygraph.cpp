/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dependencygraph.hpp"

using namespace icinga;

std::mutex DependencyGraph::m_Mutex;
DependencyGraph::DependencyMap DependencyGraph::m_Dependencies;

void DependencyGraph::AddDependency(Object *parent, Object *child)
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	if (auto [it, inserted] = m_Dependencies.insert(Edge(parent, child)); !inserted) {
		m_Dependencies.modify(it, [](Edge& e) { e.count++; });
	}
}

void DependencyGraph::RemoveDependency(Object *parent, Object *child)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	if (auto it(m_Dependencies.find(Edge(parent, child))); it != m_Dependencies.end()) {
		if (it->count == 1) {
			m_Dependencies.erase(it);
			return;
		}

		m_Dependencies.modify(it, [](Edge& e) { e.count--; });
	}
}

std::vector<Object::Ptr> DependencyGraph::GetParents(const Object::Ptr& child)
{
	std::vector<Object::Ptr> objects;

	std::unique_lock<std::mutex> lock(m_Mutex);
	auto [begin, end] = m_Dependencies.get<2>().equal_range(child.get());
	std::transform(begin, end, std::back_inserter(objects), [](const Edge& edge) {
		return Object::Ptr(edge.parent);
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
std::vector<Object::Ptr> DependencyGraph::GetChildren(const Object::Ptr& parent)
{
	std::vector<Object::Ptr> objects;

	std::unique_lock<std::mutex> lock(m_Mutex);
	auto [begin, end] = m_Dependencies.get<1>().equal_range(parent.get());
	std::transform(begin, end, std::back_inserter(objects), [](const Edge& edge) {
		return Object::Ptr(edge.child);
	});

	return objects;
}
