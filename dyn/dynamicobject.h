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

class DynamicObject : public Object
{
public:
	typedef shared_ptr<DynamicObject> Ptr;
	typedef weak_ptr<DynamicObject> WeakPtr;

	void AddParentObject(DynamicObject::Ptr parent);
	void RemoveParentObject(DynamicObject::Ptr parent);

	void AddChildObject(DynamicObject::WeakPtr parent);
	void RemoveChildObject(DynamicObject::WeakPtr parent);

	DynamicDictionary::Ptr GetProperties(void) const;
	void SetProperties(DynamicDictionary::Ptr properties);

	Dictionary::Ptr GetResolvedProperties(void) const;

	string GetName(void) const;
	string GetType(void) const;
	bool IsLocal(void) const;
	bool IsAbstract(void) const;

	void Commit(void);

protected:
	virtual void Reload(Dictionary::Ptr resolvedProperties);

private:
	set<DynamicObject::Ptr> m_Parents;
	set<DynamicObject::WeakPtr> m_Children;
	DynamicDictionary::Ptr m_Properties;

	string m_Type;
	string m_Name;
	bool m_Local;
	bool m_Abstract;

	void SetName(string name);
	void SetType(string type);
	void SetLocal(bool local);
	void SetAbstract(bool abstract);
};

}

#endif /* DYNAMICOBJECT_H */
