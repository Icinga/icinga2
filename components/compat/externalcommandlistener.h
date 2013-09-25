/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef EXTERNALCOMMANDLISTENER_H
#define EXTERNALCOMMANDLISTENER_H

#include "base/dynamicobject.h"
#include "base/objectlock.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/thread/thread.hpp>
#include <iostream>

namespace icinga
{

/**
 * @ingroup compat
 */
class ExternalCommandListener : public DynamicObject
{
public:
	DECLARE_PTR_TYPEDEFS(ExternalCommandListener);

protected:
	virtual void Start(void);

	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	String m_CommandPath;

#ifndef _WIN32
	boost::thread m_CommandThread;

	void CommandPipeThread(const String& commandPath);
#endif /* _WIN32 */

	String GetCommandPath(void) const;
};

}

#endif /* EXTERNALCOMMANDLISTENER_H */
