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

#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

namespace icinga
{

/**
 * A configuration object.
 *
 * @ingroup base
 */
class I2_BASE_API ConfigObject : public Object
{
public:
	typedef shared_ptr<ConfigObject> Ptr;
	typedef weak_ptr<ConfigObject> WeakPtr;

	typedef function<ConfigObject::Ptr (const Dictionary::Ptr&)> Factory;

	typedef map<string, Factory> ClassMap;
	typedef map<string, ConfigObject::Ptr> NameMap;
	typedef map<string, NameMap> TypeMap;

	ConfigObject(const Dictionary::Ptr& properties);

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

	ScriptTask::Ptr InvokeMethod(const string& hook,
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

	static ConfigObject::Ptr GetObject(const string& type, const string& name);
	static pair<TypeMap::iterator, TypeMap::iterator> GetTypes(void);
	static pair<NameMap::iterator, NameMap::iterator> GetObjects(const string& type);

	static void DumpObjects(const string& filename);
	static void RestoreObjects(const string& filename);

	static void RegisterClass(const string& type, Factory factory);
	static ConfigObject::Ptr Create(const string& type, const Dictionary::Ptr& properties);

	static boost::signal<void (const ConfigObject::Ptr&)> OnCommitted;
	static boost::signal<void (const ConfigObject::Ptr&)> OnRemoved;

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
	RegisterClassHelper(const string& name, ConfigObject::Factory factory)
	{
		ConfigObject::RegisterClass(name, factory);
	}
};

#define REGISTER_CLASS(klass) \
	static RegisterClassHelper g_Register ## klass(#klass, boost::make_shared<klass, const Dictionary::Ptr&>);

}

#endif /* CONFIGOBJECT_H */
