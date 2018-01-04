/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/dependencygraph.hpp"

using namespace icinga;

boost::mutex DependencyGraph::m_Mutex;
std::map<Object *, std::map<Object *, int> > DependencyGraph::m_Dependencies;

void DependencyGraph::AddDependency(Object *parent, Object *child)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Dependencies[child][parent]++;
}

void DependencyGraph::RemoveDependency(Object *parent, Object *child)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	auto& refs = m_Dependencies[child];
	auto it = refs.find(parent);

	if (it == refs.end())
		return;

	it->second--;

	if (it->second == 0)
		refs.erase(it);

	if (refs.empty())
		m_Dependencies.erase(child);
}

std::vector<Object::Ptr> DependencyGraph::GetParents(const Object::Ptr& child)
{
	std::vector<Object::Ptr> objects;

	boost::mutex::scoped_lock lock(m_Mutex);
	auto it = m_Dependencies.find(child.get());

	if (it != m_Dependencies.end()) {
		typedef std::pair<Object *, int> kv_pair;
		for (const kv_pair& kv : it->second) {
			objects.emplace_back(kv.first);
		}
	}

	return objects;
}
