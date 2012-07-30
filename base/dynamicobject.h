/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef DYNAMICOBJECT_H
#define DYNAMICOBJECT_H

namespace icinga
{

/**
 * A dynamic object that can be instantiated from the configuration file.
 *
 * @ingroup base
 */
class I2_BASE_API DynamicObject : public Object
{
public:
	typedef shared_ptr<DynamicObject> Ptr;
	typedef weak_ptr<DynamicObject> WeakPtr;

	typedef function<DynamicObject::Ptr (const Dictionary::Ptr&)> Factory;

	typedef map<string, Factory> ClassMap;
	typedef map<string, DynamicObject::Ptr> NameMap;
	typedef map<string, NameMap> TypeMap;

	DynamicObject(const Dictionary::Ptr& properties);

	void SetProperties(const Dictionary::Ptr& config);
	Dictionary::Ptr GetProperties(void) const;

	template<typename T>
	bool GetProperty(const string& key, T *value) const
	{
		return GetProperties()->Get(key, value);
	}

	void SetTags(const Dictionary::Ptr& tags);
	Dictionary::Ptr GetTags(void) const;

	template<typename T>
	void SetTag(const string& key, const T& value)
	{
		GetTags()->Set(key, value);
	}

	template<typename T>
	bool GetTag(const string& key, T *value) const
	{
		return GetTags()->Get(key, value);
	}

	void RemoveTag(const string& key);

	ScriptTask::Ptr InvokeMethod(const string& method,
	    const vector<Variant>& arguments, ScriptTask::CompletionCallback callback);

	string GetType(void) const;
	string GetName(void) const;

	bool IsLocal(void) const;
	bool IsAbstract(void) const;

	void SetSource(const string& value);
	string GetSource(void) const;

	double GetCommitTimestamp(void) const;

	void Commit(void);
	void Unregister(void);

	static DynamicObject::Ptr GetObject(const string& type, const string& name);
	static pair<TypeMap::iterator, TypeMap::iterator> GetTypes(void);
	static pair<NameMap::iterator, NameMap::iterator> GetObjects(const string& type);

	static void DumpObjects(const string& filename);
	static void RestoreObjects(const string& filename);

	static void RegisterClass(const string& type, Factory factory);
	static DynamicObject::Ptr Create(const string& type, const Dictionary::Ptr& properties);

	static boost::signal<void (const DynamicObject::Ptr&)> OnCommitted;
	static boost::signal<void (const DynamicObject::Ptr&)> OnRemoved;

private:
	static ClassMap& GetClasses(void);
	static TypeMap& GetAllObjects(void);

	Dictionary::Ptr m_Properties;
	Dictionary::Ptr m_Tags;

	static map<pair<string, string>, Dictionary::Ptr> m_PersistentTags;

	void SetCommitTimestamp(double ts);
};

class RegisterClassHelper
{
public:
	RegisterClassHelper(const string& name, DynamicObject::Factory factory)
	{
		DynamicObject::RegisterClass(name, factory);
	}
};

#define REGISTER_CLASS(klass) \
	static RegisterClassHelper g_Register ## klass(#klass, boost::make_shared<klass, const Dictionary::Ptr&>);

}

#endif /* DYNAMICOBJECT_H */
