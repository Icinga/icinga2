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

class I2_BASE_API ConfigObject : public Object
{
public:
	typedef shared_ptr<ConfigObject> Ptr;
	typedef weak_ptr<ConfigObject> WeakPtr;

	typedef ObjectMap<pair<string, string>, ConfigObject::Ptr> TNMap;
	typedef ObjectMap<string, ConfigObject::Ptr> TMap;
	typedef ObjectSet<ConfigObject::Ptr> Set;

	ConfigObject(Dictionary::Ptr properties, const Set::Ptr& container = Set::Ptr());
	ConfigObject(string type, string name, const Set::Ptr& container = Set::Ptr());

	void SetProperties(Dictionary::Ptr config);
	Dictionary::Ptr GetProperties(void) const;

	template<typename T>
	void SetProperty(const string& key, const T& value)
	{
		GetProperties()->SetProperty(key, value);
	}

	template<typename T>
	bool GetProperty(const string& key, T *value) const
	{
		return GetProperties()->GetProperty(key, value);
	}

	Dictionary::Ptr GetTags(void) const;

	template<typename T>
	void SetTag(const string& key, const T& value)
	{
		GetTags()->SetProperty(key, value);
	}

	template<typename T>
	bool GetTag(const string& key, T *value) const
	{
		return GetTags()->GetProperty(key, value);
	}

	string GetType(void) const;
	string GetName(void) const;

	void SetLocal(bool value);
	bool IsLocal(void) const;

	void SetAbstract(bool value);
	bool IsAbstract(void) const;

	void Commit(void);
	void Unregister(void);

	static ObjectSet<ConfigObject::Ptr>::Ptr GetAllObjects(void);

	static TNMap::Ptr GetObjectsByTypeAndName(void);
	static TMap::Ptr GetObjectsByType(void);

	static ConfigObject::Ptr GetObject(string type, string name);
	
	static TMap::Range GetObjects(string type);

	static function<bool (ConfigObject::Ptr)> MakeTypePredicate(string type);

private:
	Set::Ptr m_Container;
	Dictionary::Ptr m_Properties;
	Dictionary::Ptr m_Tags;

	static bool TypeAndNameGetter(const ConfigObject::Ptr& object, pair<string, string> *key);
   	static bool TypePredicate(const ConfigObject::Ptr& object, string type);

	static bool TypeGetter(const ConfigObject::Ptr& object, string *key);
};

}

#endif /* CONFIGOBJECT_H */
