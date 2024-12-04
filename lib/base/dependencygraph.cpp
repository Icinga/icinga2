/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dependencygraph.hpp"

using namespace icinga;

std::mutex DependencyGraph::m_Mutex;
std::map<Object *, std::map<Object *, int> > DependencyGraph::m_Dependencies;

void DependencyGraph::AddDependency(Object* child, Object* parent)
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_Dependencies[parent][child]++;
}

void DependencyGraph::RemoveDependency(Object* child, Object* parent)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	auto& refs = m_Dependencies[parent];
	auto it = refs.find(child);

	if (it == refs.end())
		return;

	it->second--;

	if (it->second == 0)
		refs.erase(it);

	if (refs.empty())
		m_Dependencies.erase(parent);
}

std::vector<Object::Ptr> DependencyGraph::GetChildren(const Object::Ptr& parent)
{
	std::vector<Object::Ptr> objects;

	std::unique_lock<std::mutex> lock(m_Mutex);
	auto it = m_Dependencies.find(parent.get());

	if (it != m_Dependencies.end()) {
		typedef std::pair<Object *, int> kv_pair;
		for (const kv_pair& kv : it->second) {
			objects.emplace_back(kv.first);
		}
	}

	return objects;
}
