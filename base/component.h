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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef COMPONENT_H
#define COMPONENT_H

namespace icinga
{

class I2_BASE_API Component : public Object
{
private:
	Application::WeakPtr m_Application;
	ConfigObject::Ptr m_Config;

public:
	typedef shared_ptr<Component> Ptr;
	typedef weak_ptr<Component> WeakPtr;

	void SetApplication(const Application::WeakPtr& application);
	Application::Ptr GetApplication(void) const;

	void SetConfig(const ConfigObject::Ptr& componentConfig);
	ConfigObject::Ptr GetConfig(void) const;

	virtual string GetName(void) const = 0;
	virtual void Start(void) = 0;
	virtual void Stop(void) = 0;
};

typedef Component *(*CreateComponentFunction)(void);

#define EXPORT_COMPONENT(klass) \
	extern "C" I2_EXPORT icinga::Component *CreateComponent(void)	\
	{								\
		return new klass();					\
	}

}

#endif /* COMPONENT_H */
