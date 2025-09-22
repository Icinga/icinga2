/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "base/i2-base.hpp"
#include "base/configobject.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
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
	static std::vector<ConfigObject::Ptr> GetParents(const ConfigObject::Ptr& child);
	static std::vector<ConfigObject::Ptr> GetChildren(const ConfigObject::Ptr& parent);

private:
	DependencyGraph();

	/**
	 * Represents an undirected dependency edge between two objects.
	 *
	 * It allows to traverse the graph in both directions, i.e. from parent to child and vice versa.
	 */
	struct Edge
	{
		ConfigObject* parent; // The parent object of the child one.
		ConfigObject* child; // The dependent object of the parent.
		// Counter for the number of parent <-> child edges to allow duplicates.
		int count;

		Edge(ConfigObject* parent, ConfigObject* child, int count = 1): parent(parent), child(child), count(count)
		{
		}

		struct Hash
		{
			/**
			 * Generates a unique hash of the given Edge object.
			 *
			 * Note, the hash value is generated only by combining the hash values of the parent and child pointers.
			 *
			 * @param edge The Edge object to be hashed.
			 *
			 * @return size_t The resulting hash value of the given object.
			 */
			size_t operator()(const Edge& edge) const
			{
				size_t seed = 0;
				boost::hash_combine(seed, edge.parent);
				boost::hash_combine(seed, edge.child);

				return seed;
			}
		};

		struct Equal
		{
			/**
			 * Compares whether the two Edge objects contain the same parent and child pointers.
			 *
			 * Note, the member property count is not taken into account for equality checks.
			 *
			 * @param a The first Edge object to compare.
			 * @param b The second Edge object to compare.
			 *
			 * @return bool Returns true if the two objects are equal, false otherwise.
			 */
			bool operator()(const Edge& a, const Edge& b) const
			{
				return a.parent == b.parent && a.child == b.child;
			}
		};
	};

	using DependencyMap = boost::multi_index_container<
		Edge, // The value type we want to sore in the container.
		boost::multi_index::indexed_by<
			// The first indexer is used for lookups by the Edge from child to parent, thus it
			// needs its own hash function and comparison predicate.
			boost::multi_index::hashed_unique<boost::multi_index::identity<Edge>, Edge::Hash, Edge::Equal>,
			// These two indexers are used for lookups by the parent and child pointers.
			boost::multi_index::hashed_non_unique<boost::multi_index::member<Edge, ConfigObject*, &Edge::parent>>,
			boost::multi_index::hashed_non_unique<boost::multi_index::member<Edge, ConfigObject*, &Edge::child>>
		>
	>;

	static std::mutex m_Mutex;
	static DependencyMap m_Dependencies;
};

}

#endif /* DEPENDENCYGRAPH_H */
