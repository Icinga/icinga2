/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/value.hpp"
#include <boost/range/iterator.hpp>
#include <map>
#include <shared_mutex>
#include <vector>

namespace icinga
{

typedef std::vector<std::pair<String, Value> > DictionaryData;

/**
 * A container that holds key-value pairs.
 *
 * @ingroup base
 */
class Dictionary final : public Object
{
public:
	DECLARE_OBJECT(Dictionary);

	/**
	 * An iterator that can be used to iterate over dictionary elements.
	 */
	typedef std::map<String, Value>::iterator Iterator;

	typedef std::map<String, Value>::size_type SizeType;

	typedef std::map<String, Value>::value_type Pair;

	Dictionary() = default;
	Dictionary(const DictionaryData& other);
	Dictionary(DictionaryData&& other);
	Dictionary(std::initializer_list<Pair> init);

	Value Get(const String& key) const;
	bool Get(const String& key, Value *result) const;
	void Set(const String& key, Value value, bool overrideFrozen = false);
	bool Contains(const String& key) const;

	Iterator Begin();
	Iterator End();

	size_t GetLength() const;

	void Remove(const String& key);

	void Remove(Iterator it);

	void Clear();

	void CopyTo(const Dictionary::Ptr& dest) const;
	Dictionary::Ptr ShallowClone() const;

	std::vector<String> GetKeys() const;

	static Object::Ptr GetPrototype();

	Object::Ptr Clone() const override;

	String ToString() const override;

	void Freeze();

	Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const override;
	void SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;
	bool HasOwnField(const String& field) const override;
	bool GetOwnField(const String& field, Value *result) const override;

private:
	std::map<String, Value> m_Data; /**< The data for the dictionary. */
	mutable std::shared_timed_mutex m_DataMutex;
	bool m_Frozen{false};
};

Dictionary::Iterator begin(const Dictionary::Ptr& x);
Dictionary::Iterator end(const Dictionary::Ptr& x);

}

extern template class std::map<icinga::String, icinga::Value>;

#endif /* DICTIONARY_H */
