// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/objectlock.hpp"
#include "base/shared-object.hpp"
#include "base/value.hpp"
#include "base/debuginfo.hpp"
#include <atomic>
#include <map>
#include <vector>
#include <memory>
#include <shared_mutex>

namespace icinga
{

struct NamespaceValue
{
	Value Val;
	bool Const;
};


/**
 * A namespace.
 *
 * ## External Locking
 *
 * Synchronization is handled internally, so almost all functions are safe for concurrent use without external locking.
 * The only exception to this are functions returning an iterator. To use these, the caller has to acquire an ObjectLock
 * on the namespace. The iterators only remain valid for as long as that ObjectLock is held. Note that this also
 * includes range-based for loops.
 *
 * If consistency across multiple operations is required, an ObjectLock must also be acquired to prevent concurrent
 * modifications.
 *
 * ## Internal Locking
 *
 * Two mutex objects are involved in locking a namespace: the recursive mutex inherited from the Object class that is
 * acquired and released using the ObjectLock class and the m_DataMutex shared mutex contained directly in the
 * Namespace class. The ObjectLock is used to synchronize multiple write operations against each other. The shared mutex
 * is only used to ensure the consistency of the m_Data data structure.
 *
 * Read operations must acquire a shared lock on m_DataMutex. This prevents concurrent writes to that data structure
 * but still allows concurrent reads.
 *
 * Write operations must first obtain an ObjectLock and then a shared lock on m_DataMutex. This order is important for
 * preventing deadlocks. The ObjectLock prevents concurrent write operations while the shared lock prevents concurrent
 * read operations.
 *
 * External read access to iterators is synchronized by the caller holding an ObjectLock. This ensures no concurrent
 * write operations as these require the ObjectLock but still allows concurrent reads as m_DataMutex is not locked.
 *
 * @ingroup base
 */
class Namespace final : public Object
{
public:
	DECLARE_OBJECT(Namespace);

	typedef std::map<String, NamespaceValue>::iterator Iterator;

	typedef std::map<String, NamespaceValue>::value_type Pair;

	explicit Namespace(bool constValues = false);

	Value Get(const String& field) const;
	bool Get(const String& field, Value *value) const;
	void Set(const String& field, const Value& value, bool isConst = false, const DebugInfo& debugInfo = DebugInfo());
	bool Contains(const String& field) const;
	void Remove(const String& field);
	void Freeze();
	bool Frozen() const;
	ObjectLock LockIfRequired();

	Iterator Begin();
	Iterator End();

	size_t GetLength() const;

	Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const override;
	void SetFieldByName(const String& field, const Value& value, const DebugInfo& debugInfo) override;
	bool HasOwnField(const String& field) const override;
	bool GetOwnField(const String& field, Value *result) const override;

	static Object::Ptr GetPrototype();

private:
	std::shared_lock<std::shared_timed_mutex> ReadLockUnlessFrozen() const;

	std::map<String, NamespaceValue> m_Data;
	mutable std::shared_timed_mutex m_DataMutex;
	bool m_ConstValues;
	std::atomic<bool> m_Frozen;
};

Namespace::Iterator begin(const Namespace::Ptr& x);
Namespace::Iterator end(const Namespace::Ptr& x);

}

extern template class std::map<icinga::String, std::shared_ptr<icinga::NamespaceValue> >;

#endif /* NAMESPACE_H */
