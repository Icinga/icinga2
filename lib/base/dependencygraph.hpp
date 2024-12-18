/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "base/i2-base.hpp"
#include "base/configobject.hpp"
#include <map>
#include <mutex>

namespace icinga {

/**
 * A graph that tracks dependencies between objects.
 *
 * @ingroup base
 */
class DependencyGraph
{
public:
	static void AddDependency(ConfigObject* child, ConfigObject* parent);
	static void RemoveDependency(ConfigObject* child, ConfigObject* parent);
	static std::vector<ConfigObject::Ptr> GetChildren(const ConfigObject::Ptr& parent);

private:
	DependencyGraph();

	static std::mutex m_Mutex;
	static std::map<ConfigObject*, std::map<ConfigObject*, int>> m_Dependencies;
};

}

#endif /* DEPENDENCYGRAPH_H */
