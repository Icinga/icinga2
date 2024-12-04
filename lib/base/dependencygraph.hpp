/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
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
	static void AddDependency(Object *parent, Object *child);
	static void RemoveDependency(Object *parent, Object *child);
	static std::vector<Object::Ptr> GetParents(const Object::Ptr& child);

private:
	DependencyGraph();

	/**
	 * Represents an undirected dependency edge between two objects.
	 *
	 * It allows to traverse the graph in both directions, i.e. from parent to child and vice versa.
	 */
	struct Edge
	{
		// Note, due the existing confusing implementation of the DependencyGraph class, the terms parent and
		// child are flipped here. Meaning, the parent is the dependent object and the child is the dependency.
		// Example: A service object is dependent on a host object, thus the service object is the *parent* and
		// the host object is the *child* one :).
		Object* parent;
		Object* child;
		// Is used to track the number of dependent edges of the current one.
		// A number <= 1 means, this isn't referenced by anyone and might soon be erased from the container.
		// Otherwise, each remove operation will only decrement this by 1 till it reaches 1 and causes the
		// edge to completely be erased from the container.
		int count;

		Edge(Object* parent, Object* child, int count = 1): parent(parent), child(child), count(count)
		{
		}

		struct HashPredicate
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
			// The first indexer is used for lookups by the Edge object itself, thus it
			// needs its own hash function and comparison predicate.
			boost::multi_index::hashed_non_unique<boost::multi_index::identity<Edge>, Edge::HashPredicate, Edge::HashPredicate>,
			// The second and third indexers are used for lookups by the parent and child pointers.
			boost::multi_index::hashed_non_unique<boost::multi_index::member<Edge, Object*, &Edge::parent>>,
			boost::multi_index::hashed_non_unique<boost::multi_index::member<Edge, Object*, &Edge::child>>
		>
	>;

	static std::mutex m_Mutex;
	static DependencyMap m_Dependencies;
};

}

#endif /* DEPENDENCYGRAPH_H */
