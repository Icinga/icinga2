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

#ifndef COMPONENT_H
#define COMPONENT_H

namespace icinga
{

/**
 * Interface for application extensions.
 *
 * @ingroup base
 */
class I2_BASE_API IComponent : public Object
{
public:
	typedef shared_ptr<IComponent> Ptr;
	typedef weak_ptr<IComponent> WeakPtr;

	virtual void Start(void);
	virtual void Stop(void);

protected:
	DynamicObject::Ptr GetConfig(void) const;

private:
	DynamicObject::WeakPtr m_Config; /**< The configuration object for this
				      component. */

	friend class Component;
};

/**
 * An application extension that can be dynamically loaded
 * at run-time.
 *
 * @ingroup base
 */
class I2_BASE_API Component : public DynamicObject
{
public:
	typedef shared_ptr<Component> Ptr;
	typedef weak_ptr<Component> WeakPtr;

	typedef function<IComponent::Ptr (void)> Factory;

	Component(const Dictionary::Ptr& properties);
	~Component(void);

	virtual void Start(void);

	static void AddSearchDir(const String& componentDirectory);

	static void Register(const String& name, const Factory& factory);

private:
	IComponent::Ptr m_Impl; /**< The implementation object for this
				     component. */

	static map<String, Factory> m_Factories;
};

/**
 * Helper class for registering Component implementation classes.
 *
 * @ingroup base
 */
class RegisterComponentHelper
{
public:
	RegisterComponentHelper(const String& name, const Component::Factory& factory)
	{
		Component::Register(name, factory);
	}
};

/**
 * Factory function for IComponent-based classes.
 *
 * @ingroup base
 */
template<typename T>
IComponent::Ptr ComponentFactory(void)
{
	return boost::make_shared<T>();
}


#define REGISTER_COMPONENT(name, klass) \
	static RegisterComponentHelper g_RegisterSF_ ## type(name, ComponentFactory<klass>)

}

#endif /* COMPONENT_H */
