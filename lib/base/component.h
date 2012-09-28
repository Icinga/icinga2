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

	Component(const Dictionary::Ptr& properties);
	~Component(void);

	virtual void Start(void);

	static void AddSearchDir(const String& componentDirectory);

private:
	IComponent::Ptr m_Impl; /**< The implementation object for this
				     component. */
};

typedef IComponent *(*CreateComponentFunction)(void);

#ifdef _WIN32
#	define SYM_CREATECOMPONENT(component) CreateComponent
#else /* _WIN32 */
#	define SYM_CREATECOMPONENT(component) component ## _LTX_CreateComponent
#endif /* _WIN32 */

/**
 * Implements the loader function for a component.
 *
 * @param component The name of the component.
 * @param klass The component class.
 */
#define EXPORT_COMPONENT(component, klass) \
	extern "C" I2_EXPORT icinga::IComponent *SYM_CREATECOMPONENT(component)(void) \
	{								\
		return new klass();					\
	}

}

#endif /* COMPONENT_H */
